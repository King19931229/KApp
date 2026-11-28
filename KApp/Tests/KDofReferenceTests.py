#!/usr/bin/env python3

import math
import re
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CIRCULAR_HEADER = ROOT / "Product" / "Shaders" / "dof" / "circular.h"
BRACKETING_HEADER = ROOT / "Product" / "Shaders" / "dof" / "bracketing.h"
CHECKS = 0


def check(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise AssertionError(message)


def check_close(actual, expected, message, tolerance=1.0e-9):
    check(math.isclose(actual, expected, rel_tol=tolerance, abs_tol=tolerance),
          f"{message}: actual={actual}, expected={expected}")


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def half(value):
    try:
        return struct.unpack("<e", struct.pack("<e", value))[0]
    except OverflowError:
        return math.copysign(float("inf"), value)


def unorm8(value):
    if not math.isfinite(value):
        return 0.0
    clamped = min(max(value, 0.0), 1.0)
    return round(clamped * 255.0) / 255.0


def parse_constants():
    text = CIRCULAR_HEADER.read_text(encoding="utf-8")
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)

    btext = BRACKETING_HEADER.read_text(encoding="utf-8")
    btext = re.sub(r"/\*.*?\*/", "", btext, flags=re.S)
    btext = re.sub(r"//.*", "", btext)

    radius_match = re.search(r"const\s+int\s+KERNEL_RADIUS\s*=\s*(\d+)\s*;", text)
    count_match = re.search(r"const\s+int\s+KERNEL_COUNT\s*=\s*(\d+)\s*;", text)
    check(radius_match is not None, "KERNEL_RADIUS is missing")
    check(count_match is not None, "KERNEL_COUNT is missing")
    radius = int(radius_match.group(1))
    count = int(count_match.group(1))
    check(count == radius * 2 + 1, "kernel dimensions are inconsistent")

    def parse_vec2(src, name):
        match = re.search(rf"const\s+vec2\s+{name}\s*=\s*vec2\(([^)]*)\)", src)
        check(match is not None, f"{name} is missing")
        values = [float(value) for value in match.group(1).split(",")]
        check(len(values) == 2, f"{name} must contain two values")
        return complex(values[0], values[1])

    def parse_vec2_tuple(src, name):
        v = parse_vec2(src, name)
        return (v.real, v.imag)

    def parse_vec4(src, name):
        match = re.search(rf"const\s+vec4\s+{name}\s*=\s*vec4\(([^)]*)\)", src)
        check(match is not None, f"{name} is missing")
        values = [float(value) for value in match.group(1).split(",")]
        check(len(values) == 4, f"{name} must contain four values")
        return tuple(values)

    def parse_kernel(name, encoded=False):
        match = re.search(rf"const\s+vec4\s+{name}\[\]\s*=\s*vec4\[\]\((.*?)\);", text, flags=re.S)
        check(match is not None, f"{name} is missing")
        values = []
        for value in re.findall(r"vec4\(([^)]*)\)", match.group(1)):
            numbers = [float(number) for number in value.split(",")]
            check(len(numbers) == 4, f"{name} element must contain four values")
            values.append(complex(numbers[2], numbers[3]) if encoded else complex(numbers[0], numbers[1]))
        check(len(values) == count, f"{name} must contain {count} taps")
        return values

    return {
        "radius": radius,
        "count": count,
        "near": ([parse_kernel("Kernel0_RealX_ImY_RealZ_ImW_1")],
                 [parse_vec2(text, "Kernel0Weights_RealX_ImY_1")]),
        "far": ([parse_kernel("Kernel0_RealX_ImY_RealZ_ImW_2"),
                 parse_kernel("Kernel1_RealX_ImY_RealZ_ImW_2")],
                [parse_vec2(text, "Kernel0Weights_RealX_ImY_2"),
                 parse_vec2(text, "Kernel1Weights_RealX_ImY_2")]),
        "near_bracketed": (
            [parse_kernel("Kernel0_RealX_ImY_RealZ_ImW_1", encoded=True)],
            [parse_vec4(text, "Kernel0BracketsRealXY_ImZW_1")],
            [parse_vec2_tuple(btext, "Kernel0BracketedMax_1")],
        ),
        "far_bracketed": (
            [parse_kernel("Kernel0_RealX_ImY_RealZ_ImW_2", encoded=True),
             parse_kernel("Kernel1_RealX_ImY_RealZ_ImW_2", encoded=True)],
            [parse_vec4(text, "Kernel0BracketsRealXY_ImZW_2"),
             parse_vec4(text, "Kernel1BracketsRealXY_ImZW_2")],
            [parse_vec2_tuple(btext, "Kernel0BracketedMax_2"),
             parse_vec2_tuple(btext, "Kernel1BracketedMax_2")],
        ),
    }


def clamp_index(value, size):
    return min(max(value, 0), size - 1)


def encode_bracketed_hdr(h_val, total_sum, b_max, use_fp32=False):
    if not (total_sum > 0.0):
        return 0j
    if use_fp32:
        er = f32(f32(f32(h_val.real) / f32(total_sum)) / f32(b_max[0]))
        ei = f32(f32(f32(h_val.imag) / f32(total_sum)) / f32(b_max[1]))
        return complex(er, ei)
    return complex((h_val.real / total_sum) / b_max[0], (h_val.imag / total_sum) / b_max[1])


def decode_bracketed_hdr(encoded, mean, brackets, b_max, count, use_fp32=False):
    # brackets = (O_R, S_R, O_I, S_I), b_max = (B_R, B_I)
    if use_fp32:
        s = f32(f32(mean) * f32(count))
        real = f32(s * f32(f32(f32(encoded.real) * f32(b_max[0])) * f32(brackets[1]) + f32(brackets[0])))
        imag = f32(s * f32(f32(f32(encoded.imag) * f32(b_max[1])) * f32(brackets[3]) + f32(brackets[2])))
        return complex(real, imag)
    total_sum = mean * count
    real = total_sum * (encoded.real * b_max[0] * brackets[1] + brackets[0])
    imag = total_sum * (encoded.imag * b_max[1] * brackets[3] + brackets[2])
    return complex(real, imag)


def horizontal_complex_pass(image, kernel, radius, use_fp32=False):
    height = len(image)
    width = len(image[0])
    horizontal = [[[0j] * 4 for _ in range(width)] for _ in range(height)]
    for y in range(height):
        for x in range(width):
            for tap, value in enumerate(kernel):
                sx = clamp_index(x + tap - radius, width)
                for channel in range(4):
                    if use_fp32:
                        c = f32(image[y][sx][channel])
                        prev = horizontal[y][x][channel]
                        horizontal[y][x][channel] = complex(
                            f32(f32(prev.real) + f32(c * f32(value.real))),
                            f32(f32(prev.imag) + f32(c * f32(value.imag))),
                        )
                    else:
                        horizontal[y][x][channel] += image[y][sx][channel] * value
    return horizontal


def horizontal_sum_pass(image, radius, use_fp32=False):
    height = len(image)
    width = len(image[0])
    count = radius * 2 + 1
    sums = [[[0.0] * 4 for _ in range(width)] for _ in range(height)]
    for y in range(height):
        for x in range(width):
            for tap in range(count):
                sx = clamp_index(x + tap - radius, width)
                for channel in range(4):
                    if use_fp32:
                        sums[y][x][channel] = f32(sums[y][x][channel] + f32(image[y][sx][channel]))
                    else:
                        sums[y][x][channel] += image[y][sx][channel]
    return sums


def complex_filter_2d(image, kernel, radius, half_precision=False):
    height = len(image)
    width = len(image[0])
    horizontal = horizontal_complex_pass(image, kernel, radius)

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


def bracketed_hdr_dof_filter(image, raw_kernels, encoded_kernels, brackets_list, max_list, weights, radius,
                             intermediate_mode="fp64", half_output=False):
    height = len(image)
    width = len(image[0])
    count = radius * 2 + 1
    use_fp32 = intermediate_mode in ("fp32", "fp16", "unorm8")
    sums = horizontal_sum_pass(image, radius, use_fp32=use_fp32)
    means = [[[(f32(v / f32(count)) if use_fp32 else v / count) for v in pixel] for pixel in row] for row in sums]
    if intermediate_mode in ("fp16", "unorm8"):
        means = [[[half(v) for v in pixel] for pixel in row] for row in means]

    result = [[[0.0] * 4 for _ in range(width)] for _ in range(height)]
    for raw_k, enc_k, brackets, b_max, weight in zip(raw_kernels, encoded_kernels, brackets_list, max_list, weights):
        horizontal = horizontal_complex_pass(image, enc_k, radius, use_fp32=use_fp32)
        for y in range(height):
            for x in range(width):
                for channel in range(4):
                    enc = encode_bracketed_hdr(horizontal[y][x][channel], sums[y][x][channel], b_max, use_fp32=use_fp32)
                    if intermediate_mode == "fp16":
                        enc = complex(half(enc.real), half(enc.imag))
                    elif intermediate_mode == "unorm8":
                        enc = complex(unorm8(enc.real), unorm8(enc.imag))
                    horizontal[y][x][channel] = enc

        for y in range(height):
            for x in range(width):
                accum = [0j] * 4
                for tap, k_val in enumerate(raw_k):
                    sy = clamp_index(y + tap - radius, height)
                    for channel in range(4):
                        decoded = decode_bracketed_hdr(horizontal[sy][x][channel], means[sy][x][channel],
                                                       brackets, b_max, count, use_fp32=use_fp32)
                        accum[channel] += decoded * k_val
                for channel in range(4):
                    result[y][x][channel] += resolve_complex(accum[channel], weight)
    if half_output:
        result = [[list(map(half, pixel)) for pixel in row] for row in result]
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


def make_test_image(width, height, scale=1.0):
    image = []
    for y in range(height):
        row = []
        for x in range(width):
            weight = 0.1 + 0.9 * (0.5 + 0.5 * math.sin(x * 0.37 + y * 0.61))
            color = (
                scale * (0.2 + 0.8 * (0.5 + 0.5 * math.sin(x * 0.19 + y * 0.13))),
                scale * (0.3 + 1.1 * (0.5 + 0.5 * math.sin(x * 0.23 - y * 0.17))),
                scale * (0.1 + 1.7 * (0.5 + 0.5 * math.cos(x * 0.11 + y * 0.29))),
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


def test_hdr_bracketing_math(constants):
    radius = constants["radius"]
    count = constants["count"]
    max_const_rounding = 0.0

    # 1. 自动核对 O, S, G' 及 bracketing.h 中的 B 常量等于各列最大值且大于 0。
    for name in ("near", "far"):
        raw_kernels, _ = constants[name]
        enc_kernels, brackets_list, max_list = constants[name + "_bracketed"]
        for comp, (raw_k, enc_k, brackets, b_max) in enumerate(zip(raw_kernels, enc_kernels, brackets_list, max_list)):
            o_r, s_r, o_i, s_i = brackets
            check_close(o_r, min(v.real for v in raw_k), f"{name}[{comp}] O_R mismatch", 2e-6)
            check_close(o_i, min(v.imag for v in raw_k), f"{name}[{comp}] O_I mismatch", 2e-6)
            check_close(s_r, sum(v.real - o_r for v in raw_k), f"{name}[{comp}] S_R mismatch", 2e-5)
            check_close(s_i, sum(v.imag - o_i for v in raw_k), f"{name}[{comp}] S_I mismatch", 2e-5)
            check_close(sum(v.real for v in enc_k), 1.0, f"{name}[{comp}] sum(G'_R) must be 1", 2e-5)
            check_close(sum(v.imag for v in enc_k), 1.0, f"{name}[{comp}] sum(G'_I) must be 1", 2e-5)

            exact_br = max(v.real for v in enc_k)
            exact_bi = max(v.imag for v in enc_k)
            check(b_max[0] > 0.0 and b_max[1] > 0.0, f"{name}[{comp}] B must be strictly positive")
            check(b_max[0] == exact_br and b_max[1] == exact_bi,
                  f"{name}[{comp}] bracketing.h B={b_max} must match exact circular.h max={(exact_br, exact_bi)}")

            for tap, (raw_v, enc_v) in enumerate(zip(raw_k, enc_k)):
                check(0.0 <= enc_v.real <= b_max[0] and 0.0 <= enc_v.imag <= b_max[1],
                      f"{name}[{comp}] tap {tap} encoded out of [0, B]")
                max_const_rounding = max(
                    max_const_rounding,
                    abs(enc_v.real * s_r + o_r - raw_v.real),
                    abs(enc_v.imag * s_i + o_i - raw_v.imag),
                )
                check_close(enc_v.real * s_r + o_r, raw_v.real, f"{name}[{comp}] tap {tap} real decode", 2e-6)
                check_close(enc_v.imag * s_i + o_i, raw_v.imag, f"{name}[{comp}] tap {tap} imag decode", 2e-6)

    # 2. float64 与 FP32 参考：验证全零、常量 0.1..10000、单点脉冲、暗背景高亮、通道差异、低 Alpha、clamp 边界。
    def const_img(rgb, alpha=1.0):
        return [[[rgb[0] * alpha, rgb[1] * alpha, rgb[2] * alpha, alpha] for _ in range(19)] for _ in range(19)]

    impulse_hdr = [[[0.0, 0.0, 0.0, 0.0] for _ in range(19)] for _ in range(19)]
    impulse_hdr[9][9] = [200.0, 50.0, 5.0, 1.0]
    impulse_hdr[0][0] = [100.0, 100.0, 100.0, 1.0]  # clamp 边界

    dark_bright = const_img((0.02, 0.02, 0.02), 0.5)
    dark_bright[9][9] = [500.0, 250.0, 100.0, 1.0]

    channel_diff = make_test_image(19, 19, scale=1.0)
    for row in channel_diff:
        for p in row:
            p[0] *= 500.0
            p[1] *= 0.5
            p[2] *= 0.05

    cases = [
        ("zero", const_img((0.0, 0.0, 0.0), 0.0)),
        ("const_0.1", const_img((0.1, 0.1, 0.1), 1.0)),
        ("const_1", const_img((1.0, 1.0, 1.0), 1.0)),
        ("const_10", const_img((10.0, 10.0, 10.0), 1.0)),
        ("const_100", const_img((100.0, 100.0, 100.0), 1.0)),
        ("const_1000", const_img((1000.0, 1000.0, 1000.0), 1.0)),
        ("const_10000", const_img((10000.0, 10000.0, 10000.0), 1.0)),
        ("low_alpha", const_img((0.8, 0.4, 0.2), 0.002)),
        ("impulse_hdr", impulse_hdr),
        ("dark_bright", dark_bright),
        ("channel_diff", channel_diff),
        ("mixed_hdr", make_test_image(19, 19, scale=50.0)),
    ]

    for label, img in cases:
        sums = horizontal_sum_pass(img, radius)
        means = [[[v / count for v in pixel] for pixel in row] for row in sums]
        for name in ("near", "far"):
            raw_kernels, weights = constants[name]
            enc_kernels, brackets_list, max_list = constants[name + "_bracketed"]
            for raw_k, enc_k, brackets, b_max in zip(raw_kernels, enc_kernels, brackets_list, max_list):
                h_raw = horizontal_complex_pass(img, raw_k, radius)
                h_enc = horizontal_complex_pass(img, enc_k, radius)
                for y in range(len(img)):
                    for x in range(len(img[0])):
                        for c in range(4):
                            e = encode_bracketed_hdr(h_enc[y][x][c], sums[y][x][c], b_max)
                            check(-1e-9 <= e.real <= 1.0 + 1e-9 and -1e-9 <= e.imag <= 1.0 + 1e-9,
                                  f"{label}/{name} HDR E out of [0,1]: {e}")
                            dec = decode_bracketed_hdr(e, means[y][x][c], brackets, b_max, count)
                            scale_ref = max(1.0, abs(h_raw[y][x][c].real), abs(h_raw[y][x][c].imag), sums[y][x][c])
                            check(abs(dec.real - h_raw[y][x][c].real) <= 5e-5 * scale_ref and
                                  abs(dec.imag - h_raw[y][x][c].imag) <= 5e-5 * scale_ref,
                                  f"{label}/{name} H decode mismatch at {x},{y},{c}: dec={dec} raw={h_raw[y][x][c]}")

            raw_out = dof_filter(img, raw_kernels, weights, radius)
            br_fp64 = bracketed_hdr_dof_filter(img, raw_kernels, enc_kernels, brackets_list, max_list, weights, radius, "fp64")
            br_fp32 = bracketed_hdr_dof_filter(img, raw_kernels, enc_kernels, brackets_list, max_list, weights, radius, "fp32")
            for y in range(len(img)):
                for x in range(len(img[0])):
                    for c in range(4):
                        ref_scale = max(1.0, abs(raw_out[y][x][c]), sums[y][x][c])
                        check(abs(br_fp64[y][x][c] - raw_out[y][x][c]) <= 1e-4 * ref_scale,
                              f"{label}/{name} fp64 V mismatch")
                        check(abs(br_fp32[y][x][c] - raw_out[y][x][c]) <= 2e-4 * ref_scale,
                              f"{label}/{name} fp32 V mismatch")

    print(f"HDR Bracketing kernel constant max rounding error: {max_const_rounding:.3e}")


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
                half_val = signed_radius_full(distance, focal, fstop * 2, focus, fov_y, full_height)
                check_close(half_val, actual * 0.5, "doubling FStop must halve unclamped radius")
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


def select_lower_median(values):
    arr = list(values)
    count = len(arr)
    assert 1 <= count <= 25
    k = (count - 1) // 2
    for i in range(k + 1):
        min_idx = i
        for j in range(i + 1, count):
            if arr[j] < arr[min_idx]:
                min_idx = j
        arr[i], arr[min_idx] = arr[min_idx], arr[i]
    return arr[k]


def update_focus_u_ref(focus_u, focus_state, target_u, dt, smooth_time, start_threshold, snap_threshold):
    if smooth_time <= 0.0:
        return target_u, 0
    err = abs(target_u - focus_u)
    if focus_state == 0:
        if err <= start_threshold:
            return focus_u, 0
        focus_state = 1
    if err <= snap_threshold:
        return target_u, 0
    ratio = dt / smooth_time
    alpha = ratio * (1.0 - 0.5 * ratio) if ratio < 1e-4 else (1.0 - math.exp(-ratio))
    alpha = max(0.0, min(1.0, alpha))
    new_u = focus_u + (target_u - focus_u) * alpha
    new_u = max(min(focus_u, target_u), min(max(focus_u, target_u), new_u))
    if abs(new_u - target_u) <= snap_threshold:
        return target_u, 0
    return new_u, 1


def test_autofocus_algorithm():
    # 1. Partial selection sort median: shuffled 1..25 -> 13, 13 near + 12 far, duplicates, even lower median
    shuffled = [17, 3, 25, 9, 1, 22, 13, 6, 19, 11, 24, 2, 15, 8, 20, 4, 14, 10, 23, 5, 18, 12, 21, 7, 16]
    check(select_lower_median(shuffled) == 13, "shuffled 1..25 median must be 13")
    check(select_lower_median([2.0] * 13 + [50.0] * 12) == 2.0, "13 near + 12 far must select near")
    check(select_lower_median([2.0] * 12 + [50.0] * 13) == 50.0, "12 near + 13 far must select far")
    for even_n in range(2, 25, 2):
        vals = [float(i + 1) * 3.0 for i in range(even_n)]
        check(select_lower_median(list(reversed(vals))) == vals[(even_n - 1) // 2],
              "even valid count must select lower median without averaging")
    check(select_lower_median([4.0, 4.0, 4.0, 100.0]) == 4.0, "duplicate depths lower median")

    # 2. Reciprocal-space interpolation: 2 -> 10 at 50% progress in u gives 3.333333..., not 6.0
    u_start, u_end = 1.0 / 2.0, 1.0 / 10.0
    dt_half = 0.2 * math.log(2.0)
    u_mid, state = update_focus_u_ref(u_start, 0, u_end, dt_half, 0.2, 0.01, 0.001)
    check(state == 1, "large focus jump must enter Focusing")
    check_close(1.0 / u_mid, 10.0 / 3.0, "50% reciprocal progress from 2 to 10 must be 3.3333", 1e-12)
    u_rev, state_rev = update_focus_u_ref(u_end, 0, u_start, dt_half, 0.2, 0.01, 0.001)
    check(state_rev == 1, "reverse jump 10 -> 2 must enter Focusing")
    check_close(1.0 / u_rev, 10.0 / 3.0, "50% reciprocal progress from 10 to 2 must match midpoint", 1e-12)

    # 3. Frame-rate independence across 30 / 60 / 120 FPS before snap
    results_by_fps = []
    for fps in (30, 60, 120):
        u, st = 1.0 / 2.0, 0
        dt = 1.0 / fps
        for _ in range(int(round(0.2 * fps))):
            u, st = update_focus_u_ref(u, st, 1.0 / 10.0, dt, 0.2, 0.01, 0.001)
        check(st == 1, "still focusing before snap")
        results_by_fps.append(u)
    for u in results_by_fps:
        expected = u_start + (u_end - u_start) * (1.0 - math.exp(-1.0))
        check_close(u, expected, "30/60/120 FPS must yield identical focusU at t=0.2s", 1e-12)

    # 4. Deadzone, Hysteresis (no early stop inside [snap, start]), and finite snap time
    # E0 = 0.020, tau = 0.2, start = 0.010, snap = 0.001 -> theoretical t_snap = 0.2 * ln(20) ~= 0.59915s
    u, st = 0.050, 0
    u, st = update_focus_u_ref(u, st, 0.058, 1.0 / 60.0, 0.2, 0.010, 0.001)
    check(u == 0.050 and st == 0, "Idle error <= startThreshold must hold focus")
    target = 0.070
    saw_inside_deadzone_while_tracking = False
    snapped_frame = None
    for frame in range(1, 200):
        u, st = update_focus_u_ref(u, st, target, 1.0 / 60.0, 0.2, 0.010, 0.001)
        rem = abs(target - u)
        if st == 1 and 0.001 < rem < 0.010:
            saw_inside_deadzone_while_tracking = True
        if st == 0:
            snapped_frame = frame
            break
    check(saw_inside_deadzone_while_tracking, "Focusing must continue smoothing inside (snapThreshold, startThreshold)")
    check(snapped_frame is not None and u == target and st == 0, "must snap exactly to targetU and return Idle")
    theoretical_t = 0.2 * math.log(0.020 / 0.001)
    actual_t = snapped_frame / 60.0
    check(theoretical_t <= actual_t <= theoretical_t + 1.0 / 60.0 + 1e-9,
          "snap time must match tau * ln(E0 / snapThreshold) within 1 frame")

    # After snap: small jitter stays Idle; large jump re-triggers Focusing
    u2, st2 = update_focus_u_ref(u, st, target + 0.005, 1.0 / 60.0, 0.2, 0.010, 0.001)
    check(u2 == target and st2 == 0, "post-snap jitter <= startThreshold must stay Idle")
    u3, st3 = update_focus_u_ref(u2, st2, target + 0.020, 1.0 / 60.0, 0.2, 0.010, 0.001)
    check(st3 == 1 and u3 > target, "post-snap jump > startThreshold must re-enter Focusing")

    # 5. smoothTime <= 0 snaps immediately regardless of startThreshold
    u_inst, st_inst = update_focus_u_ref(0.050, 0, 0.052, 1.0 / 60.0, 0.0, 0.010, 0.001)
    check(u_inst == 0.052 and st_inst == 0, "smoothTime <= 0 must snap immediately ignoring startThreshold")

    # 6. Small ratio stability (avoid exp(-ratio) cancellation)
    u_small, st_small = update_focus_u_ref(0.050, 1, 0.070, 1e-6, 1.0, 0.010, 0.001)
    check(st_small == 1 and u_small > 0.050, "small deltaTime/smoothTime must make forward progress")


def main():
    constants = parse_constants()
    test_reference_convolution(constants)
    test_hdr_bracketing_math(constants)
    test_constant_color(constants)
    test_kernel_separation(constants)
    test_optical_scale()
    test_composite_and_weights()
    test_numerical_guards()
    test_autofocus_algorithm()
    print(f"KDofReferenceTests: {CHECKS} checks passed")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"KDofReferenceTests failed: {error}", file=sys.stderr)
        sys.exit(1)
