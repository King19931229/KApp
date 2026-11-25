#include "dof/common.h"
#include "dof/circular.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D inputImage;

#if defined(DOF_FAR)
layout(location = 0) out vec4 redImage;
layout(location = 1) out vec4 greenImage;
layout(location = 2) out vec4 blueImage;
layout(location = 3) out vec4 alphaImage;
#else
layout(location = 0) out vec4 redGreenImage;
layout(location = 1) out vec4 blueAlphaImage;
#endif

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(inputImage, 0);

#if defined(DOF_FAR)
	vec4 red = vec4(0.0);
	vec4 green = vec4(0.0);
	vec4 blue = vec4(0.0);
	vec4 alpha = vec4(0.0);
#else
	vec2 red = vec2(0.0);
	vec2 green = vec2(0.0);
	vec2 blue = vec2(0.0);
	vec2 alpha = vec2(0.0);
#endif

	for (int i = 0; i < KERNEL_COUNT; ++i)
	{
		int index = i - KERNEL_RADIUS;
		ivec2 samplePixel = clamp(pixel + ivec2(index, 0), ivec2(0), size - ivec2(1));
		vec4 sampleValue = texelFetch(inputImage, samplePixel, 0);

	#if defined(DOF_FAR)
		vec2 k0 = Kernel0_RealX_ImY_RealZ_ImW_2[i].xy;
		vec2 k1 = Kernel1_RealX_ImY_RealZ_ImW_2[i].xy;
		vec4 kernel = vec4(k0, k1);
#else
		vec2 kernel = Kernel0_RealX_ImY_RealZ_ImW_1[i].xy;
#endif

		red += sampleValue.r * kernel;
		green += sampleValue.g * kernel;
		blue += sampleValue.b * kernel;
		alpha += sampleValue.a * kernel;
	}

#if defined(DOF_FAR)
	redImage = red;
	greenImage = green;
	blueImage = blue;
	alphaImage = alpha;
#else
	redGreenImage = vec4(red, green);
	blueAlphaImage = vec4(blue, alpha);
#endif
}
