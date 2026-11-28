#!/usr/bin/env python3
"""Run the engine's actual DOF passes and compare readbacks with a CPU reference."""

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


def read_unorm8_image(path, width, height):
    raw = path.read_bytes()
    if len(raw) != width * height * 4:
        raise AssertionError(f"GPU UNORM8 readback size mismatch: {path}")
    pixels = [tuple(b / 255.0 for b in raw[i:i + 4]) for i in range(0, len(raw), 4)]
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


def save_hdr_comparison_png(path, ref_img, test_img, scale=6):
    # 左：基线 FP16（Reinhard+gamma），中：HDR Bracketing UNORM，右：10x 显示色差热力放大
    height, width = len(ref_img), len(ref_img[0])
    def tm(v):
        v = max(v, 0.0) if math.isfinite(v) else 0.0
        return (v / (1.0 + v)) ** (1.0 / 2.2)

    strip = []
    for y in range(height):
        row = []
        for x in range(width):
            row.append(ref_img[y][x])
        for x in range(width):
            row.append(test_img[y][x])
        for x in range(width):
            diff = tuple(min(abs(tm(test_img[y][x][c]) - tm(ref_img[y][x][c])) * 10.0, 1.0) for c in range(3))
            # 反推 Reinhard 以便 save_png 直接呈现 diff 强度
            inv = tuple(d ** 2.2 / max(1.0 - d ** 2.2, 1e-4) for d in diff)
            row.append(inv + (1.0,))
        strip.append(row)
    save_png(path, strip, scale=scale)


def fixtures(phase, profile):
    cases = []

    def add(name, **values):
        case = dict(name=name, width=32, height=20, ratio=0.5, fstop=1.0, focal=8.0,
                    focus=26.0, fov=math.pi / 4, mode="perspective", debug=0,
                    action="run", pattern="mixed", tile=0, enabled=1,
                    compute=0, bracket=0, bracket_fp16=0)
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
            for pattern in ("mixed", "tile_edge", "tile_corner", "far", "near", "focus", "tiny", "invalid"):
                add(f"{tag}_tile_{pattern}_baseline", width=65, height=37, ratio=ratio,
                    pattern=pattern, fstop=1e9 if pattern == "tiny" else 1.0)
                add(f"{tag}_tile_{pattern}_on", width=65, height=37, ratio=ratio,
                    pattern=pattern, fstop=1e9 if pattern == "tiny" else 1.0, tile=1)
            add(f"{tag}_tile_view", width=65, height=37, ratio=ratio, tile=1, debug=10)
            add(f"{tag}_tile_reload", width=65, height=37, ratio=ratio, tile=1, action="reload")
            add(f"{tag}_tile_reinit", width=65, height=37, ratio=ratio, tile=1, action="reinit")
        for index, ratio in enumerate((0.5, 0.25, 0.5, 0.25, 0.5)):
            add(f"live_switch_{index}", width=65, height=37, ratio=ratio, tile=index % 2, action="switch")

        # --- PS vs CS Preprocess 直接对比 ---
        for ratio in (0.5, 0.25):
            tag = "half" if ratio == 0.5 else "quarter"
            for width, height, pattern in (
                (1, 1, "near"), (3, 3, "mixed"), (5, 1, "mixed"), (1, 5, "mixed"),
                (33, 21, "invalid"), (65, 37, "tile_edge"), (65, 37, "tiny"),
            ):
                for tile in (0, 1):
                    add(f"prep_{tag}_{width}x{height}_{pattern}_t{tile}_ps",
                        width=width, height=height, ratio=ratio, pattern=pattern,
                        fstop=1e9 if pattern == "tiny" else 1.0, tile=tile, compute=0, action="preprocess_only")
                    add(f"prep_{tag}_{width}x{height}_{pattern}_t{tile}_cs",
                        width=width, height=height, ratio=ratio, pattern=pattern,
                        fstop=1e9 if pattern == "tiny" else 1.0, tile=tile, compute=1, action="preprocess_only")

        add("cs_notile_preserve_seed", width=33, height=21, ratio=0.5, pattern="near", tile=1, compute=1, action="preprocess_only")
        add("cs_notile_preserve_check", width=33, height=21, ratio=0.5, pattern="far", tile=0, compute=1, action="preprocess_only")

        # --- HDR Bracketing FP16 数学等价验收（H 编码范围、Mean=均值、H 解码、V 与 Final） ---
        fp16_patterns = ("focus", "hdr_c10", "hdr_c1000", "hdr_impulse", "hdr_dark_bright",
                         "hdr_channel_diff", "ldr_low_alpha", "hdr_boundary", "hdr_mixed")
        for pattern in fp16_patterns:
            for tile in (0, 1):
                add(f"hcheck_{pattern}_t{tile}_raw", width=33, height=21, ratio=0.5, pattern=pattern,
                    tile=tile, compute=0, bracket=0, bracket_fp16=0, action="horizontal_only")
                add(f"hcheck_{pattern}_t{tile}_brfp16", width=33, height=21, ratio=0.5, pattern=pattern,
                    tile=tile, compute=0, bracket=1, bracket_fp16=1, action="horizontal_only")
                add(f"brfp16_{pattern}_t{tile}_raw", width=33, height=21, ratio=0.5, pattern=pattern,
                    tile=tile, compute=0, bracket=0, bracket_fp16=0)
                add(f"brfp16_{pattern}_t{tile}_on", width=33, height=21, ratio=0.5, pattern=pattern,
                    tile=tile, compute=1, bracket=1, bracket_fp16=1)

        # --- HDR Bracketing RGBA8_UNORM 完整验收矩阵（全零、常量 0.1..10000、近 FP16 上限、脉冲、暗背景高亮、通道差异、低 Alpha、边界、混合） ---
        hdr_unorm_patterns = (
            "focus", "hdr_c01", "hdr_c1", "hdr_c10", "hdr_c100", "hdr_c1000", "hdr_c10000",
            "hdr_near_max", "hdr_impulse", "hdr_dark_bright", "hdr_channel_diff",
            "ldr_low_alpha", "hdr_boundary", "hdr_mixed",
        )
        for pattern in hdr_unorm_patterns:
            # H 中间产物直接读回：校验 UNORM E 在 [0,1]、Mean 存均值、H 解码误差
            add(f"hunorm_{pattern}_raw", width=65, height=37, ratio=0.5, pattern=pattern,
                compute=0, tile=0, bracket=0, action="horizontal_only")
            add(f"hunorm_{pattern}_unorm", width=65, height=37, ratio=0.5, pattern=pattern,
                compute=0, tile=0, bracket=1, action="horizontal_only")
            # 1/2 和 1/4 完整链路对照（V 与 Final）
            for ratio in (0.5, 0.25):
                tag = "half" if ratio == 0.5 else "quarter"
                add(f"hdrbr_{tag}_{pattern}_ref", width=65, height=37, ratio=ratio, pattern=pattern,
                    compute=0, tile=0, bracket=0)
                add(f"hdrbr_{tag}_{pattern}_unorm", width=65, height=37, ratio=ratio, pattern=pattern,
                    compute=0, tile=0, bracket=1)

        # 验证原始 FP16 路径保留有符号/负 RGB 语义，不静默裁剪负颜色
        add("signed_rgb_baseline_fp16", width=33, height=21, ratio=0.5, pattern="signed_rgb",
            compute=0, tile=0, bracket=0)

        # 全部 8 种开关组合 (compute x tile x bracket) 在 HDR 混合场景下验证
        for ratio in (0.5, 0.25):
            tag = "half" if ratio == 0.5 else "quarter"
            for compute in (0, 1):
                for tile in (0, 1):
                    for bracket in (0, 1):
                        add(f"combo_{tag}_c{compute}_t{tile}_b{bracket}", width=65, height=37, ratio=ratio,
                            pattern="hdr_mixed", compute=compute, tile=tile, bracket=bracket)
            add(f"combo_{tag}_all_reload", width=33, height=21, ratio=ratio, pattern="hdr_mixed",
                compute=1, tile=1, bracket=1, action="reload")
            add(f"combo_{tag}_all_disabled", width=33, height=21, ratio=ratio, pattern="hdr_mixed",
                compute=1, tile=1, bracket=1, enabled=0)
            add(f"combo_{tag}_all_invalid_fstop", width=33, height=21, ratio=ratio, pattern="hdr_mixed",
                compute=1, tile=1, bracket=1, fstop=0)

        # 帧边界切换与资源复用验证
        add("ps_cs_reuse_seed", width=65, height=37, ratio=0.5, pattern="hdr_mixed", compute=0, tile=1, bracket=0)
        add("ps_cs_reuse_to_cs", width=65, height=37, ratio=0.5, pattern="hdr_mixed", compute=1, tile=1, bracket=0, action="switch_ps_cs")
        add("ps_cs_reuse_to_ps", width=65, height=37, ratio=0.5, pattern="hdr_mixed", compute=0, tile=0, bracket=0, action="switch_ps_cs")
        add("bracket_switch_on", width=65, height=37, ratio=0.5, pattern="hdr_mixed", compute=1, tile=1, bracket=1, action="switch_bracket")
        add("bracket_switch_quarter", width=65, height=37, ratio=0.25, pattern="hdr_mixed", compute=1, tile=1, bracket=1, action="switch")
        add("bracket_switch_off", width=65, height=37, ratio=0.25, pattern="hdr_mixed", compute=1, tile=0, bracket=0, action="switch_bracket")

    if profile:
        # 1080p 固定 HDR 场景分 Pass GPU Timestamp 对照：空 Near (hdr_far)、局部 Near (hdr_sparse)、大面积 Near (hdr_near)
        for ratio in (0.5, 0.25):
            tag = "half" if ratio == 0.5 else "quarter"
            for pattern in ("hdr_far", "hdr_sparse", "hdr_near"):
                for compute in (0, 1):
                    for tile in (0, 1):
                        for bracket in (0, 1):
                            add(f"prof_{tag}_{pattern}_c{compute}_t{tile}_b{bracket}",
                                width=1920, height=1080, ratio=ratio, pattern=pattern,
                                compute=compute, tile=tile, bracket=bracket, action="profile")
    return cases


def source_images(case):
    scene, depth = [], []
    width, height = case["width"], case["height"]
    pattern = case["pattern"]
    factor = factor_for(case)
    const_map = {
        "hdr_c01": 0.1, "hdr_c1": 1.0, "hdr_c10": 10.0, "hdr_c100": 100.0,
        "hdr_c1000": 1000.0, "hdr_c10000": 10000.0, "hdr_near_max": 48000.0,
    }
    for y in range(height):
        scene_row, depth_row = [], []
        for x in range(width):
            color = (0.125 + ((x + y) % 2) * 3, 0.25 + (x % 3), 2 + (y % 3))
            distance = 80.0
            if pattern == "focus":
                distance = 26.0
                color = (0.25 + 0.5 * ((x + y) % 2), 0.35 + 0.3 * (x % 2), 0.2 + 0.4 * (y % 2))
            elif pattern == "near":
                distance, color = 2.0, (2.0, 0.5, 8.0)
            elif pattern == "tiny":
                distance, color = 2.0, (32752.0, 16384.0, 8192.0)
            elif pattern == "tile_edge":
                if x == TILE_SIZE * factor - 1 and y == TILE_SIZE * factor - 1:
                    distance, color = 2.0, (4.0, 1.0, 8.0)
            elif pattern == "tile_corner":
                if (x, y) in ((0, 0), (width - 1, height - 1),
                              (TILE_SIZE * factor, TILE_SIZE * factor),
                              (TILE_SIZE * factor - 1, 0)):
                    distance, color = 2.0, (2.0, 0.75, 1.5)
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
            elif pattern in const_map:
                v = const_map[pattern]
                distance = 2.0 if y < height // 2 else 80.0
                color = (v, v * 0.5, v * 0.25)
            elif pattern == "hdr_impulse":
                distance = 26.0
                color = (0.1, 0.12, 0.15)
                if x == width // 2 and y == height // 2:
                    distance, color = 2.0, (200.0, 80.0, 20.0)
                elif x == width // 4 and y == height // 2:
                    distance, color = 80.0, (40.0, 160.0, 300.0)
            elif pattern == "hdr_dark_bright":
                # 暗背景上的高亮 HDR 亮斑（含 Near 与 Far 亮斑）
                distance = 80.0 if x >= width // 2 else 2.5
                color = (0.02, 0.02, 0.025)
                if abs(x - width // 4) <= 1 and abs(y - height // 2) <= 1:
                    distance, color = 2.0, (500.0, 250.0, 100.0)
                elif abs(x - (3 * width) // 4) <= 1 and abs(y - height // 2) <= 1:
                    distance, color = 85.0, (120.0, 300.0, 600.0)
            elif pattern == "hdr_channel_diff":
                # 通道间 10000x 亮度差异，检验逐通道独立 ColorMean 尺度
                distance = 2.5 if y < height // 2 else 85.0
                u = 0.2 + 0.8 * (x / max(width - 1, 1))
                color = (500.0 * u, 0.5 * u, 0.05 * u)
            elif pattern == "ldr_low_alpha":
                distance = 25.0 if x < width // 2 else 27.2
                color = (0.85, 0.45, 0.2)
            elif pattern == "hdr_boundary":
                if x < width // 3:
                    distance, color = 2.5, (40.0, 8.0, 2.0)
                elif x < (2 * width) // 3:
                    distance, color = 26.0, (0.3, 0.5, 0.4)
                else:
                    distance, color = 90.0, (4.0, 16.0, 80.0)
            elif pattern == "hdr_mixed":
                u = x / max(width - 1, 1)
                v = y / max(height - 1, 1)
                color = (0.1 + 12.0 * u, 0.2 + 6.0 * v, 0.5 + 24.0 * ((x + y) % 3) / 2.0)
                if abs(x - width // 2) <= 2 or (x < TILE_SIZE * factor and y < TILE_SIZE * factor and (x + y) % 4 == 0):
                    distance = 2.5
                    color = (40.0, 4.0, 12.0) if (x + y) % 2 else (4.0, 48.0, 16.0)
                elif y < height // 3:
                    distance = 26.0
                else:
                    distance = 85.0
            elif pattern == "signed_rgb":
                distance = 2.0 if y < height // 2 else 80.0
                color = (-0.5, 0.75, -0.25)
            elif pattern == "hdr_far":
                distance = 85.0
                color = (0.5 + 8.0 * ((x + y) & 1), 1.5, 12.0)
            elif pattern == "hdr_sparse":
                distance = 85.0
                color = (0.5, 1.5, 6.0)
                if abs(x - width // 2) <= 16 and abs(y - height // 2) <= 16:
                    distance, color = 2.0, (32.0, 8.0, 4.0)
            elif pattern == "hdr_near":
                distance = 2.0
                color = (16.0, 4.0, 32.0)

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


def compare(actual, expected, name, relative=0.015, absolute=0.004, scale_image=None, scale_tol=0.0):
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
                extra = scale_tol * abs(scale_image[y][x][channel]) if scale_image is not None else 0.0
                if not math.isfinite(actual_value) or error > absolute + relative * abs(value) + extra:
                    raise AssertionError(f"{name}: {x},{y},{channel}: GPU={actual_value}, CPU={value}, error={error}")
                worst = max(worst, error)
    return worst


def tm_srgb8(v):
    if not math.isfinite(v) or v <= 0.0:
        return 0.0
    return 255.0 * ((v / (1.0 + v)) ** (1.0 / 2.2))


def detailed_error_stats(actual, reference, channels=3, rel_denom_floor=1e-3):
    max_abs = 0.0
    sum_abs = 0.0
    max_rel = 0.0
    sum_rel = 0.0
    rel_count = 0
    tm_max = 0.0
    tm_sum = 0.0
    se = 0.0
    peak = 1.0
    count = 0
    for row_a, row_b in zip(actual, reference):
        for pix_a, pix_b in zip(row_a, row_b):
            for c in range(channels):
                a, b = pix_a[c], pix_b[c]
                if not math.isfinite(a) or not math.isfinite(b):
                    raise AssertionError(f"Non-finite pixel encountered in stats (actual={a}, ref={b})")
                err = abs(a - b)
                max_abs = max(max_abs, err)
                sum_abs += err
                se += err * err
                peak = max(peak, abs(b))
                if abs(b) > rel_denom_floor:
                    r = err / abs(b)
                    max_rel = max(max_rel, r)
                    sum_rel += r
                    rel_count += 1
                if channels == 3:
                    de = abs(tm_srgb8(a) - tm_srgb8(b))
                    tm_max = max(tm_max, de)
                    tm_sum += de
                count += 1
    rmse = math.sqrt(se / count) if count else 0.0
    psnr = float("inf") if rmse == 0.0 else 20.0 * math.log10(peak / rmse)
    stats = dict(
        max_abs=max_abs,
        avg_abs=(sum_abs / count if count else 0.0),
        max_rel=(max_rel if rel_count else 0.0),
        avg_rel=(sum_rel / rel_count if rel_count else 0.0),
        rel_samples=rel_count,
        rel_denom_floor=rel_denom_floor,
        psnr_db=(None if math.isinf(psnr) else psnr),
    )
    if channels == 3:
        stats["tm_srgb8_max"] = tm_max
        stats["tm_srgb8_avg"] = (tm_sum / count if count else 0.0)
    return stats


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--phase", choices=("b", "c"), default="c")
    parser.add_argument("--profile", action="store_true")
    parser.add_argument("--profile-only", action="store_true", help="Run only 1080p GPU timestamp benchmarks")
    parser.add_argument("--no-validation", action="store_true", help="Disable Vulkan validation layer for clean timing runs")
    parser.add_argument("--verify-only", action="store_true", help="Compare previously completed readbacks without rerunning the engine")
    parser.add_argument("--output-dir", type=Path, default=ROOT / ".codex-diagnostics/dof-phase3-hdr/gpu")
    args = parser.parse_args()
    directory = args.output_dir.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    cases = fixtures(args.phase, args.profile or args.profile_only)
    if args.profile_only:
        cases = [c for c in cases if c["action"] == "profile"]
    manifest = directory / "manifest.txt"
    input_cache = {}
    with manifest.open("w", encoding="utf-8") as output:
        for case in cases:
            scene_path, depth_path = directory / (case["name"] + ".scene.in"), directory / (case["name"] + ".depth.in")
            signature = (case["width"], case["height"], case["pattern"],
                         factor_for(case) if case["pattern"] in ("tile_edge", "tile_corner", "hdr_mixed") else 0)
            if signature in input_cache:
                for src, dst in ((input_cache[signature][0], scene_path), (input_cache[signature][1], depth_path)):
                    if dst.exists():
                        dst.unlink()
                    try:
                        os.link(src, dst)
                    except OSError:
                        shutil.copyfile(src, dst)
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
                                  ("name", "width", "height", "ratio", "fstop", "focal", "focus", "fov",
                                   "mode", "debug", "action", "tile", "enabled", "compute", "bracket", "bracket_fp16")) + "\n")

    executable = ROOT / f"Product/Binaries/x64/{args.configuration}/KDofGpuTests.exe"
    environment = dict(os.environ)
    if args.no_validation:
        environment.pop("KAPP_VULKAN_VALIDATION", None)
    else:
        environment["KAPP_VULKAN_VALIDATION"] = "1"
    if not args.verify_only:
        with (directory / "gpu.log").open("w", encoding="utf-8") as log:
            subprocess.run([str(executable), str(manifest)], cwd=ROOT, env=environment,
                           stdout=log, stderr=subprocess.STDOUT, timeout=1800, check=True)
    log = (directory / "gpu.log").read_text(encoding="utf-8", errors="replace")
    if "VUID-" in log or "SYNC-HAZARD" in log or "[Validation][Error]" in log or "[DebugDump]" in log:
        raise AssertionError("Vulkan validation or pipeline DebugDump reported errors; inspect gpu.log")
    if "DOF UI runtime:" not in log:
        raise AssertionError("Actual renderer UI runtime switches did not complete")
    if "5-MRT, RGBA16F-storage, R16F-storage, and signed-contract fallbacks verified" not in log:
        raise AssertionError("Fallback checks did not complete")
    if "GPU AutoFocus acceptance suite verified" not in log:
        raise AssertionError("GPU AutoFocus acceptance checks did not complete")

    constants = ref.parse_constants()
    results = list(csv.DictReader((directory / "gpu-results.csv").open()))
    if len(results) != len(cases):
        raise AssertionError("GPU fixture execution did not complete every case")

    result_by_name = {}
    for case, result in zip(cases, results):
        name = case["name"]
        result_by_name[name] = (case, result)
        width, height, factor = case["width"], case["height"], factor_for(case)
        ww, wh = (width + factor - 1) // factor, (height + factor - 1) // factor
        if (int(result["work_width"]), int(result["work_height"])) != (ww, wh):
            raise AssertionError(f"{name}: working size mismatch")

        work_bytes_per_px = 56 if (case["bracket"] and case["bracket_fp16"]) else (40 if case["bracket"] else 48)
        result["theoretical_image_bytes"] = ww * wh * work_bytes_per_px + width * height * 8
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
            with (directory / (name + ".final.out")).open("rb") as image:
                for x, y in ((0, 0), (width // 2, height // 2), (width - 1, height - 1)):
                    image.seek((y * width + x) * 8)
                    pixel = struct.unpack("<4e", image.read(8))
                    if any(not math.isfinite(v) for v in pixel):
                        raise AssertionError(f"{name}: non-finite pixel in profile output")
            result["profile_only"] = True
            print(f"{name}: total={float(result['gpu_ms']):.4f} ms "
                  f"(prepPS={float(result['preprocess_ps_ms']):.4f} prepCS={float(result['preprocess_cs_ms']):.4f} "
                  f"tileCS={float(result['tile_max_cs_ms']):.4f} NH={float(result['near_h_ms']):.4f} "
                  f"NV={float(result['near_v_ms']):.4f} FH={float(result['far_h_ms']):.4f} "
                  f"FV={float(result['far_v_ms']):.4f} Comp={float(result['composite_ms']):.4f}), "
                  f"mem={int(result['image_bytes']) / 1048576:.3f} MiB")
            continue

        if case["action"] in ("preprocess_only", "horizontal_only"):
            continue

        scene, depth = source_images(case)
        valid = (case["enabled"] and case["mode"] == "perspective" and all(math.isfinite(case[key]) for key in ("fstop", "focal", "focus")) and
                 case["fstop"] > 0 and case["focus"] > case["focal"] > 0)
        actual = {layer: read_image(directory / (name + f".{layer}.out"),
                                    width if layer == "final" else ww, height if layer == "final" else wh)
                  for layer in ("near", "far", "final")}

        for layer in ("near", "far"):
            if not case["enabled"]:
                continue
            if case["action"] == "composite":
                expected = read_image(directory / (name + f".{layer}.in"), ww, wh)
            elif valid:
                prep = preprocess(case, scene, depth, layer == "near")
                if case["bracket"]:
                    raw_k, weights = constants[layer]
                    enc_k, brackets_list, max_list = constants[layer + "_bracketed"]
                    inter_mode = "fp16" if case["bracket_fp16"] else "unorm8"
                    expected = ref.bracketed_hdr_dof_filter(prep, raw_k, enc_k, brackets_list, max_list, weights,
                                                            constants["radius"], inter_mode, half_output=True)
                else:
                    expected = ref.dof_filter(prep, *constants[layer], constants["radius"], half_precision=True)
            else:
                expected = [[(0, 0, 0, 0)] * ww for _ in range(wh)]
            # 与模拟相同中间格式的 CPU 参考核对 GPU 实现本身正确性；
            # UNORM 编码存在 <=1 LSB 的 GPU/CPU 舍入边界差异，解码后误差按局部垂直窗口最大 M 等比放大 (~1e-4 * max_M)
            is_unorm_bracket = bool(case["bracket"] and not case["bracket_fp16"] and valid and case["action"] != "composite")
            abs_tol = 0.008 if is_unorm_bracket else 0.004
            rel_tol = 0.025 if is_unorm_bracket else 0.015
            scale_img = None
            if is_unorm_bracket:
                h_sums = ref.horizontal_sum_pass(prep, constants["radius"], use_fp32=True)
                scale_img = [
                    [[max(h_sums[ref.clamp_index(y + t - constants["radius"], wh)][x][c]
                          for t in range(constants["count"]))
                      for c in range(4)]
                     for x in range(ww)]
                    for y in range(wh)
                ]
            result[layer + "_max_error"] = compare(
                actual[layer], expected, name + "/" + layer,
                relative=rel_tol, absolute=abs_tol,
                scale_image=scale_img, scale_tol=(1.5e-4 if is_unorm_bracket else 0.0)
            )

        if case["tile"] and case["enabled"]:
            expected_tiles = tile_reference(case, depth, valid)
            values = list(struct.iter_unpack("<e", (directory / (name + ".tile.out")).read_bytes()))
            expected_flat = [value for row in expected_tiles for value in row]
            if len(values) != len(expected_flat):
                raise AssertionError(f"{name}: tile dimensions mismatch")
            if any(not math.isfinite(value[0]) or not 0 <= value[0] <= 1 for value in values):
                raise AssertionError(f"{name}: TileMAX must contain finite normalized weights")
            errors = [abs(a[0] - e) for a, e in zip(values, expected_flat)]
            if max(errors, default=0) > 0.001:
                raise AssertionError(f"{name}: TileMAX CPU reduction mismatch")
            if case["pattern"] == "tiny" and any(value[0] == 0 for value in values):
                raise AssertionError("Tiny positive CoC must not quantize to an empty tile")
            result["tile_max_error"] = max(errors, default=0)

        expected = expected_final(case, scene, depth, actual["near"], actual["far"], valid)
        result["final_max_error"] = compare(actual["final"], expected, name + "/final")
        if name == "signed_rgb_baseline_fp16":
            # 确认基线 FP16 完整保留负 RGB，不被静默裁剪
            if not any(pixel[0] < -0.4 for row in actual["final"] for pixel in row):
                raise AssertionError("Baseline FP16 must preserve signed/negative RGB without clamping")
        if name in ("mixed", "focus", "source_invalid", "view5", "view6", "view7", "view8", "view9",
                    "half_65x37", "quarter_65x37"):
            save_png(directory / (name + ".png"), actual["final"], 6)
        print(f"{name}: GPU/CPU passed, final max error={result['final_max_error']:.6g}")

    if args.phase == "c" and not args.profile_only:
        # 1. Tile off vs on 一致性
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

        # 2. PS vs CS Preprocess 直接对比
        for case in cases:
            if case["action"] != "preprocess_only" or not case["name"].endswith("_ps"):
                continue
            ps_name = case["name"]
            cs_name = ps_name[:-3] + "_cs"
            ww, wh = (case["width"] + factor_for(case) - 1) // factor_for(case), (case["height"] + factor_for(case) - 1) // factor_for(case)
            for layer in ("near", "far"):
                a = read_image(directory / (ps_name + f".{layer}.out"), ww, wh)
                b = read_image(directory / (cs_name + f".{layer}.out"), ww, wh)
                err = compare(b, a, f"{cs_name} vs {ps_name} ({layer})", relative=1e-3, absolute=2e-3)
                result_by_name[cs_name][1][f"ps_cs_{layer}_diff"] = err
            if case["tile"]:
                tile_a = (directory / (ps_name + ".tile.out")).read_bytes()
                tile_b = (directory / (cs_name + ".tile.out")).read_bytes()
                if tile_a != tile_b:
                    vals_a = [v[0] for v in struct.iter_unpack("<e", tile_a)]
                    vals_b = [v[0] for v in struct.iter_unpack("<e", tile_b)]
                    if max(abs(x - y) for x, y in zip(vals_a, vals_b)) > 1e-4:
                        raise AssertionError(f"{cs_name}: fused TileMAX mismatch vs standalone TileMAX")
            print(f"{cs_name}: PS vs CS Preprocess matched (tile={case['tile']})")

        seed_tile = (directory / "cs_notile_preserve_seed.tile.out").read_bytes()
        after_tile = (directory / "cs_notile_preserve_check.tile.out").read_bytes()
        if seed_tile != after_tile or not any(v[0] > 0 for v in struct.iter_unpack("<e", seed_tile)):
            raise AssertionError("CS Preprocess with Tile disabled modified retained TileMAX image!")
        print("cs_notile_preserve_check: CS without Tile left TileMAX untouched")

        # 3. HDR Bracketing FP16：检查 E ∈ [0,1]、Mean = 均值（非 Sum）、H 解码与 V/Final 等价
        count = constants["count"]
        near_brackets = constants["near_bracketed"][1][0]
        near_bmax = constants["near_bracketed"][2][0]
        far_b0, far_b1 = constants["far_bracketed"][1]
        far_max0, far_max1 = constants["far_bracketed"][2]

        def verify_horizontal_hdr(raw_name, br_name, case, is_unorm):
            ww, wh = (case["width"] + factor_for(case) - 1) // factor_for(case), (case["height"] + factor_for(case) - 1) // factor_for(case)
            read_enc = read_unorm8_image if is_unorm else read_image
            raw_h0 = read_image(directory / (raw_name + ".near_h0.out"), ww, wh)
            raw_h1 = read_image(directory / (raw_name + ".near_h1.out"), ww, wh)
            enc_h0 = read_enc(directory / (br_name + ".near_h0.out"), ww, wh)
            enc_h1 = read_enc(directory / (br_name + ".near_h1.out"), ww, wh)
            near_mean = read_image(directory / (br_name + ".near_mean.out"), ww, wh)

            # 独立 CPU 检查 ColorMean 保存的是均值而非 Sum，且 E 全在 [0, 1]（容许浮点舍入 1e-4）
            scene, depth = source_images(case)
            prep_near = preprocess(case, scene, depth, True)
            prep_far = preprocess(case, scene, depth, False)
            exp_near_mean = [[[v / count for v in p] for p in row] for row in ref.horizontal_sum_pass(prep_near, constants["radius"])]
            exp_far_mean = [[[v / count for v in p] for p in row] for row in ref.horizontal_sum_pass(prep_far, constants["radius"])]
            compare(near_mean, exp_near_mean, f"{br_name}/near_mean", relative=0.01, absolute=0.005)

            near_dec_img = [[[] for _ in range(ww)] for _ in range(wh)]
            near_raw_img = [[[] for _ in range(ww)] for _ in range(wh)]
            for y in range(wh):
                for x in range(ww):
                    m = near_mean[y][x]
                    for img in (enc_h0, enc_h1):
                        for val in img[y][x]:
                            if not math.isfinite(val) or not (-1e-4 <= val <= 1.0 + 1e-4):
                                raise AssertionError(f"{br_name}: Near encoded E out of [0,1]: {val}")
                    dec_row, raw_row = [], []
                    for c, (enc_img, raw_img, off) in enumerate(((enc_h0, raw_h0, 0), (enc_h0, raw_h0, 2), (enc_h1, raw_h1, 0), (enc_h1, raw_h1, 2))):
                        dec = ref.decode_bracketed_hdr(complex(enc_img[y][x][off], enc_img[y][x][off + 1]),
                                                       m[c], near_brackets, near_bmax, count)
                        dec_row.extend([dec.real, dec.imag])
                        raw_row.extend([raw_img[y][x][off], raw_img[y][x][off + 1]])
                    near_dec_img[y][x] = dec_row
                    near_raw_img[y][x] = raw_row

            far_mean = read_image(directory / (br_name + ".far_mean.out"), ww, wh)
            compare(far_mean, exp_far_mean, f"{br_name}/far_mean", relative=0.01, absolute=0.005)
            far_dec_img = [[[] for _ in range(ww)] for _ in range(wh)]
            far_raw_img = [[[] for _ in range(ww)] for _ in range(wh)]
            far_raws = [read_image(directory / (raw_name + f".far_h{c}.out"), ww, wh) for c in range(4)]
            far_encs = [read_enc(directory / (br_name + f".far_h{c}.out"), ww, wh) for c in range(4)]
            for y in range(wh):
                for x in range(ww):
                    dec_row, raw_row = [], []
                    for c in range(4):
                        for val in far_encs[c][y][x]:
                            if not math.isfinite(val) or not (-1e-4 <= val <= 1.0 + 1e-4):
                                raise AssertionError(f"{br_name}: Far encoded E out of [0,1]: {val}")
                        m_c = far_mean[y][x][c]
                        d0 = ref.decode_bracketed_hdr(complex(far_encs[c][y][x][0], far_encs[c][y][x][1]),
                                                      m_c, far_b0, far_max0, count)
                        d1 = ref.decode_bracketed_hdr(complex(far_encs[c][y][x][2], far_encs[c][y][x][3]),
                                                      m_c, far_b1, far_max1, count)
                        dec_row.extend([d0.real, d0.imag, d1.real, d1.imag])
                        raw_row.extend(list(far_raws[c][y][x]))
                    far_dec_img[y][x] = dec_row
                    far_raw_img[y][x] = raw_row

            near_stats = detailed_error_stats(near_dec_img, near_raw_img, channels=8)
            far_stats = detailed_error_stats(far_dec_img, far_raw_img, channels=16)
            return near_stats, far_stats

        for case in cases:
            if case["action"] == "horizontal_only" and case["name"].startswith("hcheck_") and case["name"].endswith("_raw"):
                raw_name = case["name"]
                br_name = raw_name[:-4] + "_brfp16"
                ns, fs = verify_horizontal_hdr(raw_name, br_name, case, is_unorm=False)
                if ns["max_rel"] > 0.02 and ns["max_abs"] > 0.01:
                    raise AssertionError(f"{br_name}: Near FP16 HDR H decode error too large: {ns}")
                if fs["max_rel"] > 0.02 and fs["max_abs"] > 0.01:
                    raise AssertionError(f"{br_name}: Far FP16 HDR H decode error too large: {fs}")
                print(f"{br_name}: FP16 HDR H decode matched raw (near max_abs={ns['max_abs']:.5g} max_rel={ns['max_rel']:.4g}, "
                      f"far max_abs={fs['max_abs']:.5g} max_rel={fs['max_rel']:.4g})")

            if case["name"].startswith("brfp16_") and case["name"].endswith("_raw"):
                raw_name = case["name"]
                br_name = raw_name[:-4] + "_on"
                ww, wh = (case["width"] + factor_for(case) - 1) // factor_for(case), (case["height"] + factor_for(case) - 1) // factor_for(case)
                for layer in ("near", "far", "final"):
                    w, h = (case["width"], case["height"]) if layer == "final" else (ww, wh)
                    a = read_image(directory / (raw_name + f".{layer}.out"), w, h)
                    b = read_image(directory / (br_name + f".{layer}.out"), w, h)
                    tol = 0.015 if (case["pattern"] == "ldr_low_alpha" and layer == "final") else 0.008
                    compare(b, a, f"{br_name} vs {raw_name} ({layer})", relative=0.015, absolute=tol)
                print(f"{br_name}: HDR Bracketing FP16 V/Final matched raw baseline")

        # 4. HDR Bracketing RGBA8_UNORM 完整报告与高亮截图
        hdr_h_stats = {}
        for case in cases:
            if case["action"] == "horizontal_only" and case["name"].startswith("hunorm_") and case["name"].endswith("_raw"):
                pat = case["pattern"]
                raw_name = case["name"]
                unorm_name = raw_name[:-4] + "_unorm"
                ns, fs = verify_horizontal_hdr(raw_name, unorm_name, case, is_unorm=True)
                hdr_h_stats[pat] = dict(near_h=ns, far_h=fs)
                print(f"{unorm_name}: UNORM HDR E in [0,1], Mean verified | H decode near max_abs={ns['max_abs']:.5g} avg_rel={ns['avg_rel']:.4g}, "
                      f"far max_abs={fs['max_abs']:.5g} avg_rel={fs['avg_rel']:.4g}")

        hdr_report = []
        for case in cases:
            if not (case["name"].startswith("hdrbr_") and case["name"].endswith("_ref")):
                continue
            ref_name = case["name"]
            unorm_name = ref_name[:-4] + "_unorm"
            ww, wh = (case["width"] + factor_for(case) - 1) // factor_for(case), (case["height"] + factor_for(case) - 1) // factor_for(case)
            entry = dict(name=unorm_name, pattern=case["pattern"], ratio=case["ratio"])
            if case["ratio"] == 0.5 and case["pattern"] in hdr_h_stats:
                entry["horizontal_decode"] = hdr_h_stats[case["pattern"]]
            for layer in ("near", "far", "final"):
                w, h = (case["width"], case["height"]) if layer == "final" else (ww, wh)
                a = read_image(directory / (ref_name + f".{layer}.out"), w, h)
                b = read_image(directory / (unorm_name + f".{layer}.out"), w, h)
                entry[layer] = detailed_error_stats(b, a, channels=3 if layer == "final" else 4)
                if layer == "final" and case["ratio"] == 0.5 and case["pattern"] in (
                    "hdr_dark_bright", "hdr_mixed", "hdr_channel_diff", "hdr_impulse", "hdr_boundary"
                ):
                    save_hdr_comparison_png(directory / f"hdr_compare_{case['pattern']}.png", a, b, scale=6)
            hdr_report.append(entry)
            f = entry["final"]
            print(f"{unorm_name}: final max_abs={f['max_abs']:.5g} avg_abs={f['avg_abs']:.5g} "
                  f"max_rel={f['max_rel']:.4g} avg_rel={f['avg_rel']:.4g} "
                  f"tm_sRGB8 max={f['tm_srgb8_max']:.2f} avg={f['tm_srgb8_avg']:.2f}")
        (directory / "hdr-bracketing-report.json").write_text(json.dumps(hdr_report, indent=2), encoding="utf-8")

        # 5. 三开关全组合：验证同 Bracketing 模式下 (PS/CS x Tile on/off) 四种 HDR 组合一致
        for tag in ("half", "quarter"):
            for bracket in (0, 1):
                base_name = f"combo_{tag}_c0_t0_b{bracket}"
                base_case = result_by_name[base_name][0]
                ww, wh = (base_case["width"] + factor_for(base_case) - 1) // factor_for(base_case), \
                         (base_case["height"] + factor_for(base_case) - 1) // factor_for(base_case)
                base_imgs = {layer: read_image(directory / (base_name + f".{layer}.out"),
                                               base_case["width"] if layer == "final" else ww,
                                               base_case["height"] if layer == "final" else wh)
                             for layer in ("near", "far", "final")}
                for compute in (0, 1):
                    for tile in (0, 1):
                        if compute == 0 and tile == 0:
                            continue
                        cur_name = f"combo_{tag}_c{compute}_t{tile}_b{bracket}"
                        for layer in ("near", "far", "final"):
                            w, h = (base_case["width"], base_case["height"]) if layer == "final" else (ww, wh)
                            cur = read_image(directory / (cur_name + f".{layer}.out"), w, h)
                            compare(cur, base_imgs[layer], f"{cur_name} vs {base_name} ({layer})", relative=2e-3, absolute=5e-3)
                print(f"combo_{tag}_b{bracket}: all 4 (PS/CS x Tile on/off) HDR combinations consistent")

    if not args.profile_only:
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
    print(f"KDofGpuTests: {len(cases)} cases passed" + ("" if args.no_validation else "; Vulkan synchronization validation clean"))


if __name__ == "__main__":
    main()
