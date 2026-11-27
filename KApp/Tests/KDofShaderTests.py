#!/usr/bin/env python3
"""Compile the actual DOF shaders using bindings derived from the engine's inputs."""

import argparse
import re
import shutil
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def verify_parallel_tile_reduction(spirv):
    # 直接核对编译产物的线程组、共享数组和全组同步，防止后续改回串行 Tile 扫描。
    code = spirv.read_bytes()
    words = struct.unpack(f"<{len(code) // 4}I", code)
    instructions = []
    offset = 5
    while offset < len(words):
        count, opcode = words[offset] >> 16, words[offset] & 0xffff
        if count == 0 or offset + count > len(words):
            raise AssertionError("Malformed TileMAX SPIR-V instruction")
        instructions.append((opcode, words[offset + 1:offset + count]))
        offset += count
    constants = {args[1]: args[2] for op, args in instructions if op == 43 and len(args) == 3}
    float32 = {args[0] for op, args in instructions if op == 22 and args[1] == 32}
    shared_arrays = {args[0] for op, args in instructions if op == 28 and
                     args[1] in float32 and constants.get(args[2]) == 64}
    shared_pointers = {args[0] for op, args in instructions if op == 32 and
                       args[1] == 4 and args[2] in shared_arrays}
    has_shared_max = any(op == 59 and args[0] in shared_pointers and args[2] == 4
                         for op, args in instructions)
    local_sizes = [tuple(args[2:]) for op, args in instructions if op == 16 and args[1] == 17]
    local_sizes += [tuple(constants.get(value) for value in args[2:])
                    for op, args in instructions if op == 331 and args[1] == 38]
    barriers = [args for op, args in instructions if op == 224 and
                constants.get(args[0]) == 2 and constants.get(args[1]) == 2 and
                constants.get(args[2], 0) & 0x108 == 0x108]
    if (8, 8, 1) not in local_sizes or not has_shared_max or len(barriers) < 2:
        raise AssertionError("TileMAX must use 8x8 threads, shared float[64], and workgroup memory barriers")
    print("TileMAX SPIR-V: 8x8 threads, shared float[64], workgroup barriers verified")


def binding_header():
    # 与 KShaderManager 的宏展开顺序一致，避免测试中的绑定号与引擎分叉。
    lines = (ROOT / "KRender/Interface/KShaderBinding.inl").read_text(encoding="utf-8").splitlines()
    vertex_formats = (ROOT / "KRender/Interface/KVertexFormat.inl").read_text(encoding="utf-8")
    generated = []
    binding = 0
    for line in lines:
        if '#include "KVertexFormat.inl"' in line:
            for name in re.findall(r"^VERTEX_FORMAT_SCENE\((\w+)\)", vertex_formats, re.M):
                generated.append(f"#define BINDING_{name} {binding}")
                binding += 1
        match = re.match(r"\s*(CONSTANT_BUFFER_BINDING|STORAGE_BUFFER_BINDING|TEXTURE_BINDING)\((\w+)\)", line)
        if match:
            kind, name = match.groups()
            name = "TEXTURE" + name if kind == "TEXTURE_BINDING" else name
            generated.append(f"#define BINDING_{name} {binding}")
            binding += 1
    return "\n".join(generated) + "\n"


def compile_shaders(output_dir, compiler=None):
    compiler = compiler or shutil.which("glslangValidator")
    if not compiler:
        raise RuntimeError("glslangValidator was not found; add the Vulkan SDK Bin directory to PATH")
    output_dir = Path(output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "binding_generate_code.h").write_text(binding_header(), encoding="utf-8")
    variants = [("preprocess", "dof/preprocess.frag", False),
                ("near_horizontal", "dof/horizontal.frag", False),
                ("near_vertical", "dof/vertical.frag", False),
                ("far_horizontal", "dof/horizontal.frag", True),
                ("far_vertical", "dof/vertical.frag", True),
                ("composite", "dof/composite.frag", False),
                ("tile_horizontal", "dof/horizontal.frag", "tile"),
                ("tile_vertical", "dof/vertical.frag", "tile"),
                ("near_tile_max", "dof/near_tile_max.comp", False),
                ("quad", "shading/quad.vert", False)]
    validator = shutil.which("spirv-val")
    for name, source, far in variants:
        stage = "vert" if source.endswith(".vert") else "comp" if source.endswith(".comp") else "frag"
        target = "vulkan1.3" if stage == "comp" else "vulkan1.0"
        wrapper = output_dir / f"{name}.{stage}"
        wrapper.write_text("#version 460 core\n#extension GL_GOOGLE_include_directive : require\n" +
                           ("#define DOF_NEAR_TILE 1\n" if far == "tile" else "#define DOF_FAR 1\n" if far else "") +
                           f'#include "{source}"\n', encoding="utf-8")
        spirv = output_dir / f"{name}.spv"
        subprocess.run([str(compiler), "-V", "--target-env", target, "-S", stage,
                        "-I" + str(output_dir), "-I" + str(ROOT / "Product/Shaders"),
                        "-o", str(spirv), str(wrapper)], check=True)
        if validator:
            subprocess.run([validator, "--target-env", target, str(spirv)], check=True)
        if name == "near_tile_max":
            verify_parallel_tile_reduction(spirv)
    print(f"KDofShaderTests: {len(variants)} shader variants compiled" +
          (" and SPIR-V validated" if validator else "; spirv-val unavailable"))
    return output_dir


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path,
                        default=ROOT / ".codex-diagnostics/dof-phase2/shaders")
    parser.add_argument("--compiler")
    args = parser.parse_args()
    compile_shaders(args.output_dir, args.compiler)
