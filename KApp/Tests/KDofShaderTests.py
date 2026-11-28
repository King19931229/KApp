#!/usr/bin/env python3
"""Compile the actual DOF shaders using bindings derived from the engine's inputs."""

import argparse
import re
import shutil
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def parse_spirv_instructions(spirv):
    code = spirv.read_bytes()
    words = struct.unpack(f"<{len(code) // 4}I", code)
    instructions = []
    offset = 5
    while offset < len(words):
        count, opcode = words[offset] >> 16, words[offset] & 0xffff
        if count == 0 or offset + count > len(words):
            raise AssertionError(f"Malformed SPIR-V instruction in {spirv}")
        instructions.append((opcode, words[offset + 1:offset + count]))
        offset += count
    return instructions


def inspect_compute_spirv(spirv):
    instructions = parse_spirv_instructions(spirv)
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
    bindings = {args[2] for op, args in instructions if op == 71 and len(args) >= 3 and args[1] == 33}
    return local_sizes, has_shared_max, barriers, bindings


def verify_parallel_tile_reduction(spirv, label):
    # 直接核对编译产物的线程组、共享数组和全组同步，防止后续改回串行 Tile 扫描。
    local_sizes, has_shared_max, barriers, _ = inspect_compute_spirv(spirv)
    if (8, 8, 1) not in local_sizes or not has_shared_max or len(barriers) < 2:
        raise AssertionError(f"{label} must use 8x8 threads, shared float[64], and workgroup memory barriers")
    print(f"{label} SPIR-V: 8x8 threads, shared float[64], workgroup barriers verified")


def verify_compute_preprocess_no_tile(spirv, texture4_binding):
    local_sizes, has_shared_max, barriers, bindings = inspect_compute_spirv(spirv)
    if (8, 8, 1) not in local_sizes:
        raise AssertionError("preprocess_cs must use 8x8 threads")
    if has_shared_max or barriers:
        raise AssertionError("preprocess_cs without DOF_NEAR_TILE must not perform shared reduction or barriers")
    if texture4_binding in bindings:
        raise AssertionError("preprocess_cs without DOF_NEAR_TILE must not declare tileOutput descriptor")
    print("preprocess_cs SPIR-V: 8x8 threads, no shared reduction, no tile descriptor verified")


def inspect_fragment_spirv(spirv):
    instructions = parse_spirv_instructions(spirv)
    # OpVariable storage class Output = 3; OpDecorate Location = 30, Binding = 33
    output_vars = {args[1] for op, args in instructions if op == 59 and len(args) >= 3 and args[2] == 3}
    output_locations = {args[2] for op, args in instructions
                        if op == 71 and len(args) >= 3 and args[0] in output_vars and args[1] == 30}
    bindings = {args[2] for op, args in instructions if op == 71 and len(args) >= 3 and args[1] == 33}
    return output_locations, bindings


def binding_header():
    # 与 KShaderManager 的宏展开顺序一致，避免测试中的绑定号与引擎分叉。
    lines = (ROOT / "KRender/Interface/KShaderBinding.inl").read_text(encoding="utf-8").splitlines()
    vertex_formats = (ROOT / "KRender/Interface/KVertexFormat.inl").read_text(encoding="utf-8")
    generated = []
    binding_map = {}
    binding = 0
    for line in lines:
        if '#include "KVertexFormat.inl"' in line:
            for name in re.findall(r"^VERTEX_FORMAT_SCENE\((\w+)\)", vertex_formats, re.M):
                generated.append(f"#define BINDING_{name} {binding}")
                binding_map[name] = binding
                binding += 1
        match = re.match(r"\s*(CONSTANT_BUFFER_BINDING|STORAGE_BUFFER_BINDING|TEXTURE_BINDING)\((\w+)\)", line)
        if match:
            kind, name = match.groups()
            name = "TEXTURE" + name if kind == "TEXTURE_BINDING" else name
            generated.append(f"#define BINDING_{name} {binding}")
            binding_map[name] = binding
            binding += 1
    return "\n".join(generated) + "\n", binding_map


def compile_shaders(output_dir, compiler=None):
    compiler = compiler or shutil.which("glslangValidator")
    if not compiler:
        raise RuntimeError("glslangValidator was not found; add the Vulkan SDK Bin directory to PATH")
    output_dir = Path(output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    header_text, binding_map = binding_header()
    (output_dir / "binding_generate_code.h").write_text(header_text, encoding="utf-8")
    tex0 = binding_map["TEXTURE0"]
    tex4 = binding_map["TEXTURE4"]
    tex5 = binding_map["TEXTURE5"]

    variants = [
        ("autofocus", "dof/autofocus.comp", ()),
        ("preprocess", "dof/preprocess.frag", ()),
        ("preprocess_cs", "dof/preprocess.comp", ()),
        ("preprocess_cs_tile", "dof/preprocess.comp", ("DOF_NEAR_TILE",)),
        ("near_tile_max", "dof/near_tile_max.comp", ()),
        ("near_horizontal", "dof/horizontal.frag", ()),
        ("near_vertical", "dof/vertical.frag", ()),
        ("tile_horizontal", "dof/horizontal.frag", ("DOF_NEAR_TILE",)),
        ("tile_vertical", "dof/vertical.frag", ("DOF_NEAR_TILE",)),
        ("far_horizontal", "dof/horizontal.frag", ("DOF_FAR",)),
        ("far_vertical", "dof/vertical.frag", ("DOF_FAR",)),
        ("near_horizontal_bracketed", "dof/horizontal.frag", ("DOF_BRACKETED",)),
        ("near_vertical_bracketed", "dof/vertical.frag", ("DOF_BRACKETED",)),
        ("tile_horizontal_bracketed", "dof/horizontal.frag", ("DOF_NEAR_TILE", "DOF_BRACKETED")),
        ("tile_vertical_bracketed", "dof/vertical.frag", ("DOF_NEAR_TILE", "DOF_BRACKETED")),
        ("far_horizontal_bracketed", "dof/horizontal.frag", ("DOF_FAR", "DOF_BRACKETED")),
        ("far_vertical_bracketed", "dof/vertical.frag", ("DOF_FAR", "DOF_BRACKETED")),
        ("composite", "dof/composite.frag", ()),
        ("quad", "shading/quad.vert", ()),
    ]
    validator = shutil.which("spirv-val")
    for name, source, macros in variants:
        stage = "vert" if source.endswith(".vert") else "comp" if source.endswith(".comp") else "frag"
        target = "vulkan1.3" if stage == "comp" else "vulkan1.0"
        wrapper = output_dir / f"{name}.{stage}"
        macro_lines = "".join(f"#define {macro} 1\n" for macro in macros)
        wrapper.write_text("#version 460 core\n#extension GL_GOOGLE_include_directive : require\n" +
                           macro_lines + f'#include "{source}"\n', encoding="utf-8")
        spirv = output_dir / f"{name}.spv"
        subprocess.run([str(compiler), "-V", "--target-env", target, "-S", stage,
                        "-I" + str(output_dir), "-I" + str(ROOT / "Product/Shaders"),
                        "-o", str(spirv), str(wrapper)], check=True)
        if validator:
            subprocess.run([validator, "--target-env", target, str(spirv)], check=True)
        if name == "autofocus":
            local_sizes, has_shared_max, barriers, bindings = inspect_compute_spirv(spirv)
            if (1, 1, 1) not in local_sizes or has_shared_max or barriers:
                raise AssertionError("autofocus must use 1x1x1 threads without shared reduction or workgroup barriers")
            if tex0 not in bindings or tex5 not in bindings:
                raise AssertionError("autofocus must bind gbuffer0 (TEXTURE0) and autoFocusBuffer (TEXTURE5)")
            print("autofocus SPIR-V: 1x1x1 threads, TEXTURE0 + TEXTURE5 bindings verified")
        elif name == "near_tile_max":
            verify_parallel_tile_reduction(spirv, "TileMAX")
        elif name == "preprocess_cs_tile":
            verify_parallel_tile_reduction(spirv, "PreprocessCS+TileMAX")
        elif name == "preprocess_cs":
            verify_compute_preprocess_no_tile(spirv, tex4)
        elif source == "dof/horizontal.frag":
            outputs, bindings = inspect_fragment_spirv(spirv)
            expected = ({0, 1, 2, 3, 4} if "DOF_BRACKETED" in macros else {0, 1, 2, 3}) if "DOF_FAR" in macros else \
                       ({0, 1, 2} if "DOF_BRACKETED" in macros else {0, 1})
            if outputs != expected:
                raise AssertionError(f"{name}: expected MRT locations {expected}, got {outputs}")
            if tex5 in bindings:
                raise AssertionError(f"{name}: horizontal pass must not declare AutoFocus SSBO binding")
        elif source == "dof/vertical.frag":
            _, bindings = inspect_fragment_spirv(spirv)
            if ("DOF_BRACKETED" in macros) != (tex4 in bindings):
                raise AssertionError(f"{name}: ColorMean binding mismatch (expected={('DOF_BRACKETED' in macros)})")
            if tex5 in bindings:
                raise AssertionError(f"{name}: vertical pass must not declare AutoFocus SSBO binding")
        elif source in ("dof/preprocess.frag", "dof/composite.frag"):
            _, bindings = inspect_fragment_spirv(spirv)
            if tex5 not in bindings:
                raise AssertionError(f"{name}: must bind AutoFocus SSBO (TEXTURE5)")
    print(f"KDofShaderTests: {len(variants)} shader variants compiled" +
          (" and SPIR-V validated" if validator else "; spirv-val unavailable"))
    return output_dir


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path,
                        default=ROOT / ".codex-diagnostics/dof-phase3/shaders")
    parser.add_argument("--compiler")
    args = parser.parse_args()
    compile_shaders(args.output_dir, args.compiler)
