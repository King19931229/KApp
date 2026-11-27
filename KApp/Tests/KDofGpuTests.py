#!/usr/bin/env python3
"""Run the engine's actual DOF passes and compare RGBA16F readbacks with a CPU reference."""

import argparse
import csv
import json
import math
import os
import re
import shutil
import struct
import subprocess
import zlib
from pathlib import Path

import KDofReferenceTests as ref

ROOT = Path(__file__).resolve().parents[2]
TILE_SIZE = int(re.search(r"#define DOF_TILE_SIZE (\d+)",
                         (ROOT / "Product/Shaders/dof/tile_config.h").read_text(encoding="utf-8")).group(1))


def write_image(path, image):
    with path.open("wb") as output:
        for row in image:
            for pixel in row:
                output.write(struct.pack("<4e", *pixel))


def read_image(path, width, height):
    pixels = list(struct.iter_unpack("<4e", path.read_bytes()))
    if len(pixels) != width * height:
        raise AssertionError(f"GPU readback size mismatch: {path}")
    return [pixels[y * width:(y + 1) * width] for y in range(height)]


def save_png(path, image, scale=1):
    # 从数值结果生成诊断图，RGB 以统一 Reinhard + gamma 显示；不影响计算或比较。
    height, width = len(image), len(image[0])
    raw = bytearray()
    for row in image:
        scan = bytearray([0])
        for pixel in row:
            rgb = [max(value, 0.0) if math.isfinite(value) else 0.0 for value in pixel[:3]]
            encoded = bytes(round(255 * (value / (1 + value)) ** (1 / 2.2)) for value in rgb)
            scan.extend(encoded * scale)
        raw.extend(scan * scale)

    def chunk(name, data):
        return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data))
    path.write_bytes(b"\x89PNG\r\n\x1a\n" +
                     chunk(b"IHDR", struct.pack(">2I5B", width * scale, height * scale, 8, 2, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def fixtures(phase, profile):
    cases = []

    def add(name, **values):
        case = dict(name=name, width=32, height=20, ratio=0.5, fstop=1.0, focal=8.0,
                    focus=26.0, fov=math.pi / 4, mode="perspective", debug=0,
                    action="run", pattern="mixed", tile=0, enabled=1)
        case.update(values)
        cases.append(case)

    add("focus", pattern="focus")
    add("constant_near", pattern="near")
    add("constant_far", pattern="far")
    add("mixed")
    add("fstop2", fstop=2.0)
    add("focal4", focal=4.0)
    add("source_invalid", pattern="invalid")
    for name, values in (("zero_fstop", dict(fstop=0)), ("nan_fstop", dict(fstop=float("nan"))),
                         ("inf_focus", dict(focus=float("inf"))), ("focus_equals_focal", dict(focus=8)),
                         ("ortho", dict(mode="ortho")), ("aspect", dict(mode="aspect")),
                         ("missing_camera", dict(mode="missing"))):
        add(name, **values)
    for view in range(1, 10):
        add(f"view{view}", debug=view)
    add("reload", action="reload")
    add("reinit", action="reinit")
    bad_values = [(1, 2, 3, alpha) for alpha in (-0.5, 0, 1e-6, 1e-5, 0.25, 2, float("nan"), float("inf"))]
    bad_values += [(value, 2, 3, 1) for value in (float("nan"), float("inf"))]
    for layer in ("near", "far"):
        for index, value in enumerate(bad_values):
            add(f"{layer}_guard{index}", pattern="far", action="composite", **{layer: value})
    if phase == "c":
        add("asymmetric", mode="asymmetric")
        add("nonfinite_projection", mode="nonfinite_projection")
        add("depth_invalid", pattern="depth_invalid")
        add("zero_resize", action="zero_resize")
        for ratio in (0.5, 0.25):
            tag = "half" if ratio == 0.5 else "quarter"
            for width, height in ((33, 21), (1, 1), (1, 5), (5, 1), (3, 3), (65, 37)):
                add(f"{tag}_{width}x{height}", width=width, height=height, ratio=ratio)
            add(f"{tag}_focus", width=33, height=21, ratio=ratio, pattern="focus")
            add(f"{tag}_impulse", width=35, height=35, ratio=ratio, pattern="impulse")
            add(f"{tag}_reload", width=33, height=21, ratio=ratio, action="reload")
            add(f"{tag}_reinit", width=33, height=21, ratio=ratio, action="reinit")
            add(f"{tag}_alignment", width=33, height=21, ratio=ratio, action="composite", near="ramp")
        add("unsupported_ratio", width=33, height=21, ratio=0.3)
        add("nan_ratio", width=33, height=21, ratio=float("nan"))
        add("inf_ratio", width=33, height=21, ratio=float("inf"))
        add("enable_off", enabled=0)
        add("tile_view_off", debug=10)
        for ratio in (0.5, 0.25):
            tag = "half" if ratio == 0.5 else "quarter"
            for pattern in ("mixed", "tile_edge", "far", "near", "focus", "tiny"):
                add(f"{tag}_tile_{pattern}_baseline", width=65, height=37, ratio=ratio,
                    pattern=pattern, fstop=1e9 if pattern == "tiny" else 1.0)
                add(f"{tag}_tile_{pattern}_on", width=65, height=37, ratio=ratio,
                    pattern=pattern, fstop=1e9 if pattern == "tiny" else 1.0, tile=1)
            add(f"{tag}_tile_view", width=65, height=37, ratio=ratio, tile=1, debug=10)
            add(f"{tag}_tile_reload", width=65, height=37, ratio=ratio, tile=1, action="reload")
            add(f"{tag}_tile_reinit", width=65, height=37, ratio=ratio, tile=1, action="reinit")
        for index, ratio in enumerate((0.5, 0.25, 0.5, 0.25, 0.5)):
            add(f"live_switch_{index}", width=65, height=37, ratio=ratio, tile=index % 2, action="switch")
    if profile:
        for ratio in (0.5, 0.25):
            add("profile_half" if ratio == 0.5 else "profile_quarter",
                width=1920, height=1080, ratio=ratio, pattern="near", action="profile")
            for pattern in ("far", "sparse", "near"):
                tag = "half" if ratio == 0.5 else "quarter"
                for tile in (0, 1):
                    if pattern == "near" and tile == 0: continue
                    add(f"profile_{tag}_{pattern}_tile{tile}", width=1920, height=1080, ratio=ratio,
                        pattern=pattern, action="profile", tile=tile)
    return cases


def source_images(case):
    scene, depth = [], []
    width, height = case["width"], case["height"]
    pattern = case["pattern"]
    for y in range(height):
        scene_row, depth_row = [], []
        for x in range(width):
            color = (0.125 + ((x + y) % 2) * 3, 0.25 + (x % 3), 2 + (y % 3))
            distance = 80.0
            if pattern == "focus":
                distance = 26.0
            elif pattern == "near":
                distance, color = 2.0, (2.0, 0.5, 8.0)
            elif pattern == "tiny":
                distance, color = 2.0, (32752.0, 16384.0, 8192.0)
            elif pattern == "tile_edge":
                if x == TILE_SIZE * factor_for(case) - 1 and y == TILE_SIZE * factor_for(case) - 1:
                    distance, color = 2.0, (4.0, 1.0, 8.0)
            elif pattern == "sparse":
                color = (0.5, 1.0, 2.0)
                if abs(x - width // 2) <= 1 and abs(y - height // 2) <= 1:
                    distance, color = 2.0, (4.0, 1.0, 8.0)
            elif pattern == "impulse":
                distance = 26.0
                if x == width // 2 and y == height // 2:
                    distance, color = 2.0, (4.0, 1.0, 8.0)
            elif pattern in ("mixed", "invalid") and abs(x - width // 2) <= 1:
                distance = 2.0
                color = (4.0, 0.125, 0.5) if y % 2 else (0.125, 4.0, 0.5)
            if pattern == "invalid" and (x, y) == (width // 2, height // 2):
                color = (float("nan"), float("inf"), 0)
            scene_row.append(tuple(map(ref.half, color)) + (1.0,))
            linear_depth = ref.half((distance - 1.0) / 100.0)
            if pattern == "depth_invalid" and y == height // 2:
                if x == width // 2:
                    linear_depth = float("nan")
                elif x == width // 2 + 1:
                    linear_depth = float("inf")
                elif x == width // 2 + 2:
                    linear_depth = -1.0
            depth_row.append((0.0, 0.0, 0.0, linear_depth))
        scene.append(scene_row)
        depth.append(depth_row)
    return scene, depth


def factor_for(case):
    return 4 if abs(case["ratio"] - 0.25) < 1e-6 else 2


def weight_at(case, depth):
    return ref.signed_coc(1.0 + 100.0 * depth[3], focal_length=case["focal"], fstop=case["fstop"],
                          focus_distance=case["focus"], fov_y=case["fov"], full_height=case["height"],
                          factor=factor_for(case))


def preprocess(case, scene, depth, near):
    factor = factor_for(case)
    width, height = case["width"], case["height"]
    result = []
    for by in range(0, height, factor):
        row = []
        for bx in range(0, width, factor):
            accum = [0.0] * 4
            count = min(factor, width - bx) * min(factor, height - by)
            for y in range(by, min(by + factor, height)):
                for x in range(bx, min(bx + factor, width)):
                    if not all(math.isfinite(value) for value in scene[y][x][:3]):
                        continue
                    coc = weight_at(case, depth[y][x])
                    weight = max(-coc if near else coc, 0.0)
                    for channel in range(3):
                        accum[channel] += scene[y][x][channel] * weight
                    accum[3] += weight
            row.append([ref.half(value / count) for value in accum])
        result.append(row)
    return result


def work_coordinate(position, full_size, factor):
    count = (full_size + factor - 1) // factor
    if count == 1:
        return 0.0
    coordinate = position / factor - 0.5
    previous_center = (count - 1.5) * factor
    if full_size % factor and position > previous_center:
        last_center = ((count - 1) * factor + full_size) * 0.5
        coordinate = count - 2 + (position - previous_center) / (last_center - previous_center)
    return min(max(coordinate, 0.0), count - 1.0)


def bilinear(image, x, y):
    x0, y0 = math.floor(x), math.floor(y)
    x1, y1 = min(x0 + 1, len(image[0]) - 1), min(y0 + 1, len(image) - 1)
    tx, ty = x - x0, y - y0
    return tuple((image[y0][x0][c] * (1 - tx) + image[y0][x1][c] * tx) * (1 - ty) +
                 (image[y1][x0][c] * (1 - tx) + image[y1][x1][c] * tx) * ty for c in range(4))


def signed_debug(value):
    if not math.isfinite(value):
        return (1.0, 0.0, 1.0)
    magnitude = abs(value) / (1 + abs(value))
    return (0.0, 0.0, magnitude) if value < 0 else (magnitude, 0.0, 0.0)


def tile_reference(case, depth, valid):
    # 独立 CPU 数值参考，不进入渲染路径；GPU 由 64 线程 shared-memory 并行归约。
    factor = factor_for(case)
    footprint = factor * TILE_SIZE
    result = []
    for by in range(0, case["height"], footprint):
        row = []
        for bx in range(0, case["width"], footprint):
            maximum = 0.0
            if valid:
                for y in range(by, min(by + footprint, case["height"])):
                    for x in range(bx, min(bx + footprint, case["width"])):
                        maximum = max(maximum, max(-weight_at(case, depth[y][x]), 0.0))
            row.append(ref.half(max(maximum, 2 ** -14) if maximum > 0 else 0.0))
        result.append(row)
    return result


def expected_final(case, scene, depth, near, far, valid):
    result = []
    factor, view = factor_for(case), case["debug"]
    tile = tile_reference(case, depth, valid) if case["tile"] and view == 10 else None
    for y in range(case["height"]):
        row = []
        for x in range(case["width"]):
            color = scene[y][x][:3]
            color = color if all(map(math.isfinite, color)) else (0, 0, 0)
            if not valid:
                row.append(tuple(color) + (1.0,))
                continue
            nx = work_coordinate(x + 0.5, case["width"], factor)
            ny = work_coordinate(y + 0.5, case["height"], factor)
            n, f = bilinear(near, nx, ny), bilinear(far, nx, ny)
            coc = weight_at(case, depth[y][x])
            base = ref.composite(color, f, (0, 0, 0, 0), max(coc, 0))
            value = ref.composite(color, f, n, max(coc, 0))
            alpha = min(n[3], 1.0) if all(map(math.isfinite, n)) and n[3] > 1e-5 else 0.0
            resolved = tuple(n[c] / n[3] for c in range(3)) if alpha > 0 else base
            if not ref.storable(ref.mix_color(base, resolved, alpha)):
                alpha, resolved = 0.0, base
            distance = 1.0 + 100.0 * depth[y][x][3]
            radius = ref.signed_radius_full(distance, case["focal"], case["fstop"], case["focus"], case["fov"], case["height"])
            if view == 1:
                optical = case["focal"] ** 2 / (case["fstop"] * (case["focus"] - case["focal"]))
                value = signed_debug(optical * (distance - case["focus"]) / distance)
            elif view in (2, 3):
                value = signed_debug(radius if view == 2 else radius / factor)
            elif view == 4:
                value = (max(coc, 0), 0, max(-coc, 0))
            elif view == 5:
                value = (max(-coc, 0),) * 3
            elif view == 6:
                value = signed_debug(n[3])
            elif view == 7:
                value = (alpha,) * 3
            elif view == 8:
                value = resolved if ref.storable(resolved) else (1, 0, 1)
            elif view == 9:
                value = base
            elif view == 10:
                value = (tile[y // factor // TILE_SIZE][x // factor // TILE_SIZE],) * 3 if tile else (0.0,) * 3
            row.append(tuple(map(ref.half, value)) + (1.0,))
        result.append(row)
    return result


def compare(actual, expected, name, relative=0.015, absolute=0.004):
    worst = 0.0
    for y, row in enumerate(expected):
        for x, pixel in enumerate(row):
            for channel, value in enumerate(pixel):
                actual_value = actual[y][x][channel]
                if not math.isfinite(value):
                    if math.isnan(value) and math.isnan(actual_value) or actual_value == value:
                        continue
                    raise AssertionError(f"{name}: invalid-value mismatch at {x},{y},{channel}")
                error = abs(actual_value - value)
                if not math.isfinite(actual_value) or error > absolute + relative * abs(value):
                    raise AssertionError(f"{name}: {x},{y},{channel}: GPU={actual_value}, CPU={value}, error={error}")
                worst = max(worst, error)
    return worst


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--phase", choices=("b", "c"), default="c")
    parser.add_argument("--profile", action="store_true")
    parser.add_argument("--verify-only", action="store_true", help="Compare previously completed readbacks without rerunning the engine")
    parser.add_argument("--output-dir", type=Path, default=ROOT / ".codex-diagnostics/dof-phase2/gpu")
    args = parser.parse_args()
    directory = args.output_dir.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    cases = fixtures(args.phase, args.profile)
    manifest = directory / "manifest.txt"
    input_cache = {}
    with manifest.open("w", encoding="utf-8") as output:
        for case in cases:
            scene_path, depth_path = directory / (case["name"] + ".scene.in"), directory / (case["name"] + ".depth.in")
            signature = (case["width"], case["height"], case["pattern"], factor_for(case) if case["pattern"] == "tile_edge" else 0)
            if signature in input_cache:
                shutil.copyfile(input_cache[signature][0], scene_path)
                shutil.copyfile(input_cache[signature][1], depth_path)
            else:
                scene, depth = source_images(case)
                write_image(scene_path, scene)
                write_image(depth_path, depth)
                input_cache[signature] = (scene_path, depth_path)
                del scene, depth
            if case["action"] == "composite":
                factor = factor_for(case)
                for layer in ("near", "far"):
                    ww, wh = (case["width"] + factor - 1) // factor, (case["height"] + factor - 1) // factor
                    if case.get(layer) == "ramp":
                        image = [[(float(x + 1), float(y + 1), (x + y) * 0.25, 1.0) for x in range(ww)] for y in range(wh)]
                    else:
                        pixel = tuple(map(ref.half, case.get(layer, (0, 0, 0, 0))))
                        image = [[pixel] * ww for _ in range(wh)]
                    write_image(directory / (case["name"] + f".{layer}.in"), image)
            output.write(" ".join(str(case[key]) for key in
                                  ("name", "width", "height", "ratio", "fstop", "focal", "focus", "fov", "mode", "debug", "action", "tile", "enabled")) + "\n")
    executable = ROOT / f"Product/Binaries/x64/{args.configuration}/KDofGpuTests.exe"
    environment = dict(os.environ, KAPP_VULKAN_VALIDATION="1")
    if not args.verify_only:
        with (directory / "gpu.log").open("w", encoding="utf-8") as log:
            subprocess.run([str(executable), str(manifest)], cwd=ROOT, env=environment,
                           stdout=log, stderr=subprocess.STDOUT, timeout=900, check=True)
    log = (directory / "gpu.log").read_text(encoding="utf-8", errors="replace")
    if "VUID-" in log or "SYNC-HAZARD" in log or "[Validation][Error]" in log:
        raise AssertionError("Vulkan validation reported errors; inspect gpu.log")
    if "DOF UI runtime:" not in log:
        raise AssertionError("Actual renderer UI runtime switches did not complete")
    constants = ref.parse_constants()
    results = list(csv.DictReader((directory / "gpu-results.csv").open()))
    if len(results) != len(cases):
        raise AssertionError("GPU fixture execution did not complete every case")
    for case, result in zip(cases, results):
        name = case["name"]
        width, height, factor = case["width"], case["height"], factor_for(case)
        ww, wh = (width + factor - 1) // factor, (height + factor - 1) // factor
        if (int(result["work_width"]), int(result["work_height"])) != (ww, wh):
            raise AssertionError(f"{name}: working size mismatch")
        result["theoretical_image_bytes"] = (6 * ww * wh + width * height) * 8
        tw, th = int(result.get("tile_width", 0)), int(result.get("tile_height", 0))
        if tw or th:
            if (tw, th) != ((ww + TILE_SIZE - 1) // TILE_SIZE, (wh + TILE_SIZE - 1) // TILE_SIZE):
                raise AssertionError(f"{name}: allocated TileMAX size mismatch")
            result["theoretical_image_bytes"] += tw * th * 2
        if int(result["image_bytes"]) < result["theoretical_image_bytes"]:
            raise AssertionError(f"{name}: image allocation accounting mismatch")
        if case["action"] == "profile":
            if float(result["gpu_ms"]) <= 0:
                raise AssertionError(f"{name}: GPU timestamp duration must be positive")
            worst = 0.0
            expected_pixel = (2, 0.5, 8, 1) if case["pattern"] == "near" else None
            with (directory / (name + ".final.out")).open("rb") as image:
                for x, y in ((0, 0), (width // 2, height // 2), (width - 1, height - 1)):
                    image.seek((y * width + x) * 8)
                    pixel = struct.unpack("<4e", image.read(8))
                    for channel, actual_value in enumerate(pixel):
                        if not math.isfinite(actual_value) or (expected_pixel and abs(actual_value - expected_pixel[channel]) > 0.01):
                            raise AssertionError(f"{name}: constant HDR profile output mismatch")
                        if expected_pixel: worst = max(worst, abs(actual_value - expected_pixel[channel]))
            result["constant_hdr_max_error"] = worst
            result["profile_only"] = True
            print(f"{name}: {result['gpu_ms']} ms GPU, {int(result['image_bytes']) / 1048576:.3f} MiB image requirements")
            continue
        scene, depth = source_images(case)
        valid = (case["enabled"] and case["mode"] == "perspective" and all(math.isfinite(case[key]) for key in ("fstop", "focal", "focus")) and
                 case["fstop"] > 0 and case["focus"] > case["focal"] > 0)
        actual = {layer: read_image(directory / (name + f".{layer}.out"),
                                    width if layer == "final" else ww, height if layer == "final" else wh)
                  for layer in ("near", "far", "final")}
        expected_layers = {}
        for layer in ("near", "far"):
            if not case["enabled"]:
                expected_layers[layer] = actual[layer]
                continue
            if case["action"] == "composite":
                expected = read_image(directory / (name + f".{layer}.in"), ww, wh)
            elif valid:
                expected = ref.dof_filter(preprocess(case, scene, depth, layer == "near"),
                                          *constants[layer], constants["radius"], half_precision=True)
            else:
                expected = [[(0, 0, 0, 0)] * ww for _ in range(wh)]
            expected_layers[layer] = expected
            result[layer + "_max_error"] = compare(actual[layer], expected, name + "/" + layer)
        if case["tile"] and case["enabled"]:
            expected_tiles = tile_reference(case, depth, valid)
            values = list(struct.iter_unpack("<e", (directory / (name + ".tile.out")).read_bytes()))
            expected_flat = [value for row in expected_tiles for value in row]
            if len(values) != len(expected_flat):
                raise AssertionError(f"{name}: tile dimensions mismatch")
            if any(not math.isfinite(value[0]) or not 0 <= value[0] <= 1 for value in values):
                raise AssertionError(f"{name}: TileMAX must contain finite normalized weights")
            errors = [abs(actual[0] - expected) for actual, expected in zip(values, expected_flat)]
            if max(errors, default=0) > 0.001:
                raise AssertionError(f"{name}: TileMAX CPU reduction mismatch")
            if case["pattern"] == "tiny" and any(value[0] == 0 for value in values):
                raise AssertionError("Tiny positive CoC must not quantize to an empty tile")
            result["tile_max_error"] = max(errors, default=0)
        # 卷积单独对照 CPU；合成使用实际读回 N/F，避免小分母放大量化误差而掩盖具体故障阶段。
        expected = expected_final(case, scene, depth, actual["near"], actual["far"], valid)
        result["final_max_error"] = compare(actual["final"], expected, name + "/final")
        if name in ("mixed", "focus", "source_invalid", "view5", "view6", "view7", "view8", "view9",
                    "half_65x37", "quarter_65x37"):
            save_png(directory / (name + ".png"), actual["final"], 6)
        print(f"{name}: GPU/CPU passed, final max error={result['final_max_error']:.6g}")
    if args.phase == "c":
        for case in cases:
            if not case["name"].endswith("_baseline"): continue
            baseline = case["name"]
            enabled = baseline[:-len("baseline")] + "on"
            ww, wh = (case["width"] + factor_for(case) - 1) // factor_for(case), (case["height"] + factor_for(case) - 1) // factor_for(case)
            for layer in ("near", "far", "final"):
                width, height = (case["width"], case["height"]) if layer == "final" else (ww, wh)
                a = read_image(directory / (baseline + f".{layer}.out"), width, height)
                b = read_image(directory / (enabled + f".{layer}.out"), width, height)
                if case["pattern"] == "tiny" and layer == "near" and not any(pixel[0] != 0 for row in b for pixel in row):
                    raise AssertionError("Tiny-CoC culling must preserve finite premultiplied RGB even when FP16 alpha is zero")
                for row_a, row_b in zip(a, b):
                    for pixel_a, pixel_b in zip(row_a, row_b):
                        for value_a, value_b in zip(pixel_a, pixel_b):
                            if abs(value_a - value_b) > 2 * max(2 ** -24, abs(value_a) / 1024):
                                raise AssertionError(f"{enabled}: Tile on/off mismatch in {layer}")
    width, height = 32, 20
    far1 = read_image(directory / "mixed.far.out", width // 2, height // 2)
    for name, ratio in (("fstop2", 0.5), ("focal4", (4 / 22) / (8 / 18))):
        far2 = read_image(directory / (name + ".far.out"), width // 2, height // 2)
        errors = [abs(far2[y][x][3] / far1[y][x][3] - ratio)
                  for y in range(height // 2) for x in range(width // 2) if far1[y][x][3] > 1e-3]
        if not errors or max(errors) > 0.003:
            raise AssertionError(f"{name}: actual GPU optical scale response is incorrect")
    views = {view: read_image(directory / f"view{view}.final.out", width, height) for view in (5, 7, 8, 9)}
    scene, _ = source_images(next(case for case in cases if case["name"] == "mixed"))
    final = read_image(directory / "mixed.final.out", width, height)
    x, y = width // 2, height // 2
    diagnosis = dict(pixel=[x, y], near_coc=views[5][y][x][0], near_alpha=views[7][y][x][0],
                     near_result=views[8][y][x][:3], base=views[9][y][x][:3], final=final[y][x][:3])
    if diagnosis["base"] != scene[y][x][:3] or not 0 < diagnosis["near_alpha"] < diagnosis["near_coc"]:
        raise AssertionError("Thin foreground fixture did not expose the expected Base/coverage relationship")
    (directory / "ghosting-diagnosis.json").write_text(json.dumps(diagnosis, indent=2), encoding="utf-8")
    strip = [list(scene[y]) + list(views[8][y]) + list(views[9][y]) + list(final[y]) for y in range(height)]
    save_png(directory / "ghosting-comparison.png", strip, 6)
    (directory / "verification.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(f"KDofGpuTests: {len(cases)} cases passed; Vulkan synchronization validation clean")


if __name__ == "__main__":
    main()
