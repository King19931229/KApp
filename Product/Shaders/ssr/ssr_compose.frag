#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D ssrColor;
layout(binding = BINDING_TEXTURE1) uniform sampler2D variance;

layout(location = 0) out vec4 finalImage;

void main()
{
	finalImage = texture(ssrColor, screenCoord);
	// finalImage = texture(variance, screenCoord);
}