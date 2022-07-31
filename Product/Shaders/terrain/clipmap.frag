layout(location = 0) in vec2 inUV;
layout(location = 1) in float inLerp;
layout(location = 2) in float inHeight;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(inHeight, 0.0, 0.0, 1.0);
}