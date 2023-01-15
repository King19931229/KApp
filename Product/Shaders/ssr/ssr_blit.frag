#include "public.h"

layout(location = 0) in vec2 screenCoord;
layout(binding = BINDING_TEXTURE0) uniform sampler2D inputImage;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(inputImage, screenCoord);
}