#!/usr/bin/env python3
"""Compile production SVO shaders and the independent GPU regression fixtures."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess

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
    sources = sorted(p for p in (shaders / 'voxel/svo').rglob('*') if p.suffix in ('.vert', '.geom', '.frag', '.comp'))
    sources += sorted(p for p in (shaders / 'tests').glob('svo_*') if p.suffix in ('.vert', '.geom', '.frag', '.comp'))
    variants = [(p, '#define SVO_MIP 1\n#define SVO_SLICE 0\n' if p.name == 'svo_mip_sample.frag' else '') for p in sources]
    variants += [(shaders / 'tests/svo_volume_upload.comp', '#define SVO_R8 1\n')]
    variants += [(shaders / 'tests/svo_volume_read.comp', '#define SVO_SCREEN 1\n')]
    for index, (source, macros) in enumerate(variants):
        relative = source.relative_to(shaders).as_posix()
        stage = source.suffix[1:]
        label = relative.replace('/', '_') + ('_variant%d' % index if macros else '')
        wrapper = output / (label + '.' + stage)
        wrapper.write_text('#version 460 core\n#extension GL_GOOGLE_include_directive : require\n' + macros +
                           '#include "' + relative + '"\n', encoding='utf-8')
        spirv = output / (label + '.spv')
        result = subprocess.run([compiler, '-V', '--target-env', 'vulkan1.0', '-S', stage,
                                 '-I' + str(output), '-I' + str(shaders), '-o', str(spirv), str(wrapper)],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        if result.returncode:
            raise AssertionError(relative + '\n' + result.stdout)
        subprocess.run([validator, '--target-env', 'vulkan1.0', str(spirv)], check=True)
        print('PASS ' + relative + (' (' + macros.strip().replace('#define ', '') + ')' if macros else ''))
    print('KSvoShaderTests: %d production/fixture variants passed' % len(variants))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', type=Path, default=ROOT / '.codex-diagnostics/svo/shaders')
    compile_shaders(parser.parse_args().output_dir.resolve())
