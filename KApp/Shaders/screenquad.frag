#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

float LinearizeDepth(float depth)
{
  float n = 1.0; // camera z near
  float f = 3000.0; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main()
{
	float depth = texture(texSampler, uv).r;
	outColor = vec4(vec3(1.0-LinearizeDepth(depth)), 1.0);
}