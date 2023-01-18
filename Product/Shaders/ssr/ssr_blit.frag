#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D finalImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D finalSquaredImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D finalTsppImage;

layout(location = 0) out vec4 outFinal;
layout(location = 1) out vec4 outSquaredFinal;
layout(location = 2) out vec4 outTsppFinal;

void main()
{
	outFinal = texture(finalImage, screenCoord);
	outSquaredFinal = texture(finalSquaredImage, screenCoord);
	outTsppFinal = texture(finalTsppImage, screenCoord);
}