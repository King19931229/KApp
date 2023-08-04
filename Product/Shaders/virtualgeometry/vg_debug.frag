layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inPrevWorldPos;
layout(location = 3) in vec3 inWorldNormal;
layout(location = 4) in vec3 inVertexColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inVertexColor, 1);
}