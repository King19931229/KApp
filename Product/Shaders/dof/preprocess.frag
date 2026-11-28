// Near/Far premultiplied layer preprocess.
#include "dof/preprocess_common.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D sceneColor;

layout(location = 0) out vec4 nearColor;
layout(location = 1) out vec4 farColor;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	// 对焦距离、光学直径系数、全分辨率像素半径系数、工作/全分辨率像素尺度。
	vec4 dofParams;
	// 参数有效标志、诊断视图、Tile 生效标志、保留。
	vec4 dofParams2;
} object;

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	DofPreprocessResult result = DofPreprocessWorkPixel(
		gbuffer0, sceneColor, pixel, object.dofParams, object.dofParams2);
	nearColor = result.nearColor;
	farColor = result.farColor;
}
