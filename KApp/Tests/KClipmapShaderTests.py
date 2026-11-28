#!/usr/bin/env python3
"""Compile the production Clipmap shaders and validate their SPIR-V."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def binding_header():
    lines = (ROOT / 'KRender/Interface/KShaderBinding.inl').read_text(encoding='utf-8').splitlines()
    vertices = (ROOT / 'KRender/Interface/KVertexFormat.inl').read_text(encoding='utf-8')
    generated = []
    binding = 0
    for line in lines:
        if '#include "KVertexFormat.inl"' in line:
            for name in re.findall(r'^VERTEX_FORMAT_SCENE\((\w+)\)', vertices, re.M):
                generated.append('#define BINDING_%s %d' % (name, binding))
                binding += 1
        match = re.match(r'\s*(CONSTANT_BUFFER_BINDING|STORAGE_BUFFER_BINDING|TEXTURE_BINDING)\((\w+)\)', line)
        if match:
            kind, name = match.groups()
            name = 'TEXTURE' + name if kind == 'TEXTURE_BINDING' else name
            generated.append('#define BINDING_%s %d' % (name, binding))
            binding += 1
    semantics = (ROOT / 'KRender/Interface/KVertexSemantic.inl').read_text(encoding='utf-8')
    generated += ['#define %s %s' % pair for pair in re.findall(r'VERTEX_SEMANTIC\((\w+),\s*(\d+)\)', semantics)]
    return '\n'.join(generated) + '\n'


def compile_shaders(output):
    compiler = shutil.which('glslangValidator')
    validator = shutil.which('spirv-val')
    if not compiler or not validator:
        raise RuntimeError('glslangValidator and spirv-val must be on PATH')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'binding_generate_code.h').write_text(binding_header(), encoding='utf-8')
    shaders = ROOT / 'Product/Shaders'
    sources = sorted(p for p in (shaders / 'voxel/clipmap').rglob('*') if p.suffix in ('.vert', '.geom', '.frag', '.comp'))
    sources += [shaders / 'shadow/cascaded/static_mask.frag']
    sources += sorted(p for p in (shaders / 'tests').glob('clipmap_*') if p.suffix in ('.comp', '.frag'))
    variants = [(source, ()) for source in sources]
    upload = shaders / 'tests/clipmap_volume_upload.comp'
    readback = shaders / 'tests/clipmap_volume_read.comp'
    variants += [(upload, ('CLIPMAP_UNORM',)), (upload, ('CLIPMAP_SCREEN',)),
                 (upload, ('CLIPMAP_UNORM', 'CLIPMAP_SCREEN')), (readback, ('CLIPMAP_SCREEN',))]
    for source, macros in variants:
        relative = source.relative_to(shaders).as_posix()
        stage = source.suffix[1:]
        name = source.stem + ''.join('_' + macro for macro in macros) + source.suffix
        wrapper = output / name
        definitions = ''.join('#define ' + macro + ' 1\n' for macro in macros)
        wrapper.write_text('#version 460 core\n#extension GL_GOOGLE_include_directive : require\n' + definitions + '#include "' + relative + '"\n', encoding='utf-8')
        target = 'vulkan1.3' if stage == 'comp' else 'vulkan1.0'
        spirv = output / (name + '.spv')
        result = subprocess.run([compiler, '-V', '--target-env', target, '-S', stage,
                                 '-I' + str(output), '-I' + str(shaders), '-o', str(spirv), str(wrapper)],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        if result.returncode:
            raise AssertionError(relative + '\n' + result.stdout)
        subprocess.run([validator, '--target-env', target, str(spirv)], check=True)
        print('PASS ' + relative + (' [' + ','.join(macros) + ']' if macros else ''))
    print('KClipmapShaderTests: %d production/fixture shader variants passed' % len(variants))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    if args.output_dir:
        compile_shaders(args.output_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='kclipmap-shaders-') as output:
            compile_shaders(Path(output))
