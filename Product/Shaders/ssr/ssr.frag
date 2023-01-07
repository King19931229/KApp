#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer1;
layout(binding = BINDING_TEXTURE2) uniform sampler2D gbuffer2;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer3;
layout(binding = BINDING_TEXTURE4) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE5) uniform sampler2D hiZ;

layout(location = 0) out vec4 outColor;

#include "shading/gbuffer.h"

void main()
{
    outColor = texture(sceneColor, screenCoord);
}