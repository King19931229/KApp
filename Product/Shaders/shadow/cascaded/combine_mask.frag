layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D staticMask;
layout(binding = 1) uniform sampler2D dynamicMask;

void main()
{
	outColor = min(texture(dynamicMask, inUV), texture(staticMask, inUV));
}