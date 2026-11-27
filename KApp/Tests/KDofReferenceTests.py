#!/usr/bin/env python3

import math
import re
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CIRCULAR_HEADER = ROOT / "Product" / "Shaders" / "dof" / "circular.h"
CHECKS = 0


def check(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise AssertionError(message)


def check_close(actual, expected, message, tolerance=1.0e-9):
    check(math.isclose(actual, expected, rel_tol=tolerance, abs_tol=tolerance),
          f"{message}: actual={actual}, expected={expected}")


def parse_constants():
    text = CIRCULAR_HEADER.read_text(encoding="utf-8")
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)

    radius_match = re.search(r"const\s+int\s+KERNEL_RADIUS\s*=\s*(\d+)\s*;", text)
    count_match = re.search(r"const\s+int\s+KERNEL_COUNT\s*=\s*(\d+)\s*;", text)
    check(radius_match is not None, "KERNEL_RADIUS is missing")
    check(count_match is not None, "KERNEL_COUNT is missing")
    radius = int(radius_match.group(1))
    count = int(count_match.group(1))
    check(count == radius * 2 + 1, "kernel dimensions are inconsistent")

    def parse_vec2(name):
        match = re.search(rf"const\s+vec2\s+{name}\s*=\s*vec2\(([^)]*)\)", text)
        check(match is not None, f"{name} is missing")
        values = [float(value) for value in match.group(1).split(",")]
        check(len(values) == 2, f"{name} must contain two values")
        return complex(values[0], values[1])

    def parse_kernel(name):
        match = re.search(rf"const\s+vec4\s+{name}\[\]\s*=\s*vec4\[\]\((.*?)\);", text, flags=re.S)
        check(match is not None, f"{name} is missing")
        values = []
        for value in re.findall(r"vec4\(([^)]*)\)", match.group(1)):
            numbers = [float(number) for number in value.split(",")]
            check(len(numbers) == 4, f"{name} element must contain four values")
            values.append(complex(numbers[0], numbers[1]))
        check(len(values) == count, f"{name} must contain {count} taps")
        return values

    return {
        "radius": radius,
        "near": ([parse_kernel("Kernel0_RealX_ImY_RealZ_ImW_1")],
                 [parse_vec2("Kernel0Weights_RealX_ImY_1")]),
        "far": ([parse_kernel("Kernel0_RealX_ImY_RealZ_ImW_2"),
                 parse_kernel("Kernel1_RealX_ImY_RealZ_ImW_2")],
                [parse_vec2("Kernel0Weights_RealX_ImY_2"),
                 parse_vec2("Kernel1Weights_RealX_ImY_2")]),
    }


def clamp_index(value, size):
    return min(max(value, 0), size - 1)


def half(value):
    try:
        return struct.unpack("<e", struct.pack("<e", value))[0]
    except OverflowError:
        return math.copysign(float("inf"), value)


def complex_filter_2d(image, kernel, radius, half_precision=False):
    height = len(image)
    width = len(image[0])
    horizontal = [[[0j] * 4 for _ in range(width)] for _ in range(height)]

    for y in range(height):
        for x in range(width):
            for tap, value in enumerate(kernel):
                sx = clamp_index(x + tap - radius, width)
                for channel in range(4):
                    horizontal[y][x][channel] += image[y][sx][channel] * value

    if half_precision:
        # 实际 H 输出会经过 RGBA16F RT，V 读取的是量化后的复数。
        for row in horizontal:
            for pixel in row:
                for channel in range(4):
                    value = pixel[channel]
                    pixel[channel] = complex(half(value.real), half(value.imag))

    result = [[[0j] * 4 for _ in range(width)] for _ in range(height)]
    for y in range(height):
        for x in range(width):
            for tap, value in enumerate(kernel):
                sy = clamp_index(y + tap - radius, height)
                for channel in range(4):
                    result[y][x][channel] += horizontal[sy][x][channel] * value
    return result


def resolve_complex(value, weight):
    return value.real * weight.real + value.imag * weight.imag


def dof_filter(image, kernels, weights, radius, half_precision=False):
    height = len(image)
    width = len(image[0])
    result = [[[0.0] * 4 for _ in range(width)] for _ in range(height)]
    for kernel, weight in zip(kernels, weights):
        filtered = complex_filter_2d(image, kernel, radius, half_precision)
        for y in range(height):
            for x in range(width):
                for channel in range(4):
                    result[y][x][channel] += resolve_complex(filtered[y][x][channel], weight)
    if half_precision:
        result = [[list(map(half, pixel)) for pixel in row] for row in result]
    return result


def direct_kernel_2d(kernels, weights, radius):
    size = radius * 2 + 1
    kernel = [[0.0] * size for _ in range(size)]
    for filter_kernel, weight in zip(kernels, weights):
        for y in range(size):
            for x in range(size):
                value = filter_kernel[y] * filter_kernel[x]
                kernel[y][x] += resolve_complex(value, weight)
    return kernel


def direct_filter_2d(image, kernel, radius):
    height = len(image)
    width = len(image[0])
    result = [[[0.0] * 4 for _ in range(width)] for _ in range(height)]
    for y in range(height):
        for x in range(width):
            for tap_y in range(radius * 2 + 1):
                sy = clamp_index(y + tap_y - radius, height)
                for tap_x in range(radius * 2 + 1):
                    sx = clamp_index(x + tap_x - radius, width)
                    weight = kernel[tap_y][tap_x]
                    for channel in range(4):
                        result[y][x][channel] += image[sy][sx][channel] * weight
    return result


def make_test_image(width, height):
    image = []
    for y in range(height):
        row = []
        for x in range(width):
            weight = 0.1 + 0.9 * (0.5 + 0.5 * math.sin(x * 0.37 + y * 0.61))
            color = (
                0.2 + 0.8 * (0.5 + 0.5 * math.sin(x * 0.19 + y * 0.13)),
                0.3 + 1.1 * (0.5 + 0.5 * math.sin(x * 0.23 - y * 0.17)),
                0.1 + 1.7 * (0.5 + 0.5 * math.cos(x * 0.11 + y * 0.29)),
            )
            row.append([color[0] * weight, color[1] * weight, color[2] * weight, weight])
        image.append(row)
    return image


def test_reference_convolution(constants):
    image = make_test_image(19, 19)
    radius = constants["radius"]
    for name in ("near", "far"):
        kernels, weights = constants[name]
        separable = dof_filter(image, kernels, weights, radius)
        direct = direct_filter_2d(image, direct_kernel_2d(kernels, weights, radius), radius)
        for y in range(len(image)):
            for x in range(len(image[0])):
                for channel in range(4):
                    check_close(separable[y][x][channel], direct[y][x][channel],
                                f"{name} direct/separable mismatch at {x},{y},{channel}", 1.0e-8)


def test_constant_color(constants):
    image = make_test_image(23, 17)
    color = (0.35, 1.2, 2.4)
    for y in range(len(image)):
        for x in range(len(image[0])):
            weight = image[y][x][3]
            image[y][x] = [color[0] * weight, color[1] * weight, color[2] * weight, weight]

    for name in ("near", "far"):
        kernels, weights = constants[name]
        filtered = dof_filter(image, kernels, weights, constants["radius"])
        for y in range(len(image)):
            for x in range(len(image[0])):
                alpha = filtered[y][x][3]
                if abs(alpha) > 1.0e-5:
                    for channel in range(3):
                        check_close(filtered[y][x][channel] / alpha, color[channel],
                                    f"{name} constant color changed at {x},{y},{channel}", 1.0e-8)


def test_kernel_separation(constants):
    image = make_test_image(17, 17)
    radius = constants["radius"]
    near = dof_filter(image, *constants["near"], radius)
    far = dof_filter(image, *constants["far"], radius)
    difference = max(abs(near[y][x][c] - far[y][x][c])
                     for y in range(len(image)) for x in range(len(image[0])) for c in range(3))
    check(difference > 1.0e-5, "Near and Far references must not resolve to the same filter")


def signed_radius_full(distance, focal_length, fstop, focus_distance, fov_y, full_height):
    values = (distance, focal_length, fstop, focus_distance, fov_y, full_height)
    if (not all(math.isfinite(value) for value in values) or
            distance <= 0.0 or focal_length <= 0.0 or fstop <= 0.0 or
            focus_distance <= focal_length or not 0.0 < fov_y < math.pi or full_height <= 0):
        return 0.0
    scale = focal_length * full_height / (4.0 * fstop * (focus_distance - focal_length) * math.tan(fov_y / 2.0))
    return scale * (distance - focus_distance) / distance


def signed_coc(distance, focal_length=1.0, fstop=1.0, focus_distance=200.0,
               fov_y=math.pi / 4.0, full_height=1080, factor=2, radius=8):
    full = signed_radius_full(distance, focal_length, fstop, focus_distance, fov_y, full_height)
    return min(max(full / factor / radius, -1.0), 1.0)


def test_optical_scale():
    # 用显式成像面几何独立检查预计算系数，避免相对 CoCMax 再次抵消光学尺度。
    fov_y, full_height, focus = math.radians(53.0), 721, 200.0
    for focal in (0.5, 1.0, 12.0):
        sensor_height = 2.0 * focal * math.tan(fov_y / 2.0)
        for fstop in (0.5, 1.0, 2.0, 16.0):
            for distance in (2.0, 150.0, focus, 500.0):
                diameter = (focal / fstop) * focal * (distance - focus) / (distance * (focus - focal))
                expected = diameter * 0.5 * full_height / sensor_height
                actual = signed_radius_full(distance, focal, fstop, focus, fov_y, full_height)
                check_close(actual, expected, "pixel radius must match optical diameter geometry")
                half = signed_radius_full(distance, focal, fstop * 2, focus, fov_y, full_height)
                check_close(half, actual * 0.5, "doubling FStop must halve unclamped radius")
    r1 = signed_radius_full(500, 1, 1, focus, fov_y, full_height)
    r2 = signed_radius_full(500, 2, 1, focus, fov_y, full_height)
    check_close(r2 / r1, 2.0 * (focus - 1) / (focus - 2), "fixed-FOV focal response is not cancelled")
    check(signed_coc(100) < 0, "near CoC must be negative")
    check(signed_coc(200) == 0, "focus plane must have zero CoC")
    check(signed_coc(500) > 0, "far CoC must be positive")
    for name, value in (("distance", 0.0), ("distance", float("nan")),
                        ("fstop", 0.0), ("fstop", float("inf")),
                        ("focal_length", 0.0), ("focus_distance", 1.0),
                        ("fov_y", math.pi), ("full_height", 0)):
        args = dict(distance=500.0, focal_length=1.0, fstop=1.0,
                    focus_distance=200.0, fov_y=fov_y, full_height=full_height)
        args[name] = value
        check(signed_radius_full(**args) == 0.0, f"invalid {name} must bypass")


def mix_color(a, b, weight):
    return tuple(a[i] + (b[i] - a[i]) * weight for i in range(3))


def storable(color):
    return all(math.isfinite(value) and abs(value) <= 65504.0 for value in color)


def composite(base, far_value, near_value, far_weight):
    epsilon = 1.0e-5
    base = base if all(math.isfinite(value) for value in base) else (0.0, 0.0, 0.0)
    far_color = base
    far_blend = 0.0
    if all(math.isfinite(value) for value in far_value) and far_value[3] > epsilon:
        candidate = tuple(far_value[i] / far_value[3] for i in range(3))
        if all(math.isfinite(value) for value in candidate):
            far_color = candidate
            far_blend = far_weight
    base_color = mix_color(base, far_color, far_blend)
    if not storable(base_color):
        base_color = base

    near_color = base_color
    near_alpha = 0.0
    if all(math.isfinite(value) for value in near_value) and near_value[3] > epsilon:
        candidate = tuple(near_value[i] / near_value[3] for i in range(3))
        if all(math.isfinite(value) for value in candidate):
            near_color = candidate
            near_alpha = min(max(near_value[3], 0.0), 1.0)
    result = mix_color(base_color, near_color, near_alpha)
    return result if storable(result) else base_color


def test_composite_and_weights():
    base = (0.1, 0.2, 0.3)
    far_value = (0.4, 0.6, 0.8, 1.0)
    near_color = (1.0, 0.5, 0.25)
    near_value = tuple(component * 0.5 for component in near_color) + (0.5,)
    result = composite(base, far_value, near_value, 1.0)
    expected = mix_color(far_value[:3], near_color, 0.5)
    for channel in range(3):
        check_close(result[channel], expected[channel], "near must composite after far", 1.0e-12)

    for near_alpha in (0.0, -0.25, 1.0e-6):
        near = (1.0, 1.0, 1.0, near_alpha)
        result = composite(base, (0.0, 0.0, 0.0, 0.0), near, 0.0)
        for channel in range(3):
            check_close(result[channel], base[channel], "zero/negative near alpha must keep base", 1.0e-12)

    result = composite(base, (1.0, 1.0, 1.0, 0.0), (0.0, 0.0, 0.0, 0.0), 1.0)
    for channel in range(3):
        check_close(result[channel], base[channel], "zero far alpha must keep base", 1.0e-12)


def test_numerical_guards():
    base = (0.125, 2.0, 16.0)
    zero = (0.0, 0.0, 0.0, 0.0)
    invalid_values = [(1, 2, 3, alpha) for alpha in (-1, 0, 1e-6, 1e-5, float("nan"), float("inf"))]
    invalid_values += [(value, 2, 3, 1) for value in (float("nan"), float("inf"), -float("inf"))]
    for value in invalid_values:
        check(composite(base, value, zero, 1.0) == base, "invalid Far must keep SceneColor")
        check(composite(base, zero, value, 0.0) == base, "invalid Near must keep Base")
    hdr = (1024.0, 12.0, -0.25)
    for alpha in (1.1e-5, 0.125, 0.75, 1.0, 2.0):
        premultiplied = tuple(value * alpha for value in hdr) + (alpha,)
        result = composite(base, zero, premultiplied, 0.0)
        expected = mix_color(base, hdr, min(alpha, 1.0))
        for channel in range(3):
            check_close(result[channel], expected[channel], "only coverage may clamp; the original denominator must survive")
    check(composite((float("nan"), 0, 0), zero, zero, 0.0) == (0, 0, 0), "invalid SceneColor must use black fallback")
    # 全失焦原始 NearCoC=1 不强制覆盖率为 1；保留基线中的清晰 Base 贡献。
    near = (0.5, 1.0, 2.0, 0.5)
    expected = mix_color(base, (1.0, 2.0, 4.0), 0.5)
    check(composite(base, zero, near, 0.0) == expected, "Near baseline coverage must remain convolved alpha")
    check(composite(base, (1, 2, 3, 1.1e-5), zero, 1.0) == base,
          "finite Far resolve outside the final format range must fall back")
    check(composite(base, zero, (65504, 1, 1, 0.5), 0.0) == base,
          "Near contribution outside the final format range must keep Base")


def main():
    constants = parse_constants()
    test_reference_convolution(constants)
    test_constant_color(constants)
    test_kernel_separation(constants)
    test_optical_scale()
    test_composite_and_weights()
    test_numerical_guards()
    print(f"KDofReferenceTests: {CHECKS} checks passed")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"KDofReferenceTests failed: {error}", file=sys.stderr)
        sys.exit(1)
