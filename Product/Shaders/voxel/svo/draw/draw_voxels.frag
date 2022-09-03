layout(location = 0) in vec4 voxelColor;
layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = vec4(voxelColor);
}