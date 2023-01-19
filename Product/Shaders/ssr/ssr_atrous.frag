#include "public.h"
#define GAUSSIAN_KERNEL_5X5
#include "kernal.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "util.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D inputImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE2) uniform sampler2D varianceImage;

layout(location = 0) out vec4 finalImage;

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint level;
} object;

float DepthThreshold(float depth, vec2 ddxy, vec2 pixelOffset)
{
	float depthThreshold;
	depthThreshold = dot(vec2(1.0), abs(pixelOffset * ddxy));
	return depthThreshold;
}

void AddFilterContribution(
	in out vec4 weightedValueSum,
	in out vec4 weightSum,
	vec4 value,
	vec4 stdDeviation,
	float depth,
	vec3 normal,
	vec2 ddxy,
	uint row,
	uint col,
	vec2 uv,
	vec2 kernalStep,
	vec2 invScreenSize
)
{
 	const float valueSigma = 4.0;
	const float normalSigma = 32.0;
	const float depthSigma = 1.0;

	vec2 pixelOffset = vec2(int(row) - int(Radius), int(col) - int(Radius)) * kernalStep * invScreenSize;
	vec2 samplePos = uv + pixelOffset;

	if (samplePos.x >= 0.5 * invScreenSize.x
		&& samplePos.y >= 0.5 * invScreenSize.y
		&& samplePos.x <= (1.0 - 0.5 * invScreenSize.x)
	 	&& samplePos.y <= (1.0 - 0.5 * invScreenSize.y))
	{
		vec4 gbuffer0Data = texture(gbuffer0, screenCoord);

		vec4 iValue = texture(inputImage, samplePos);
		float iDepth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
		vec3 iNormal = normalize(DecodeNormalViewSpace(gbuffer0Data));

		// Calculate a weight for the neighbor's contribtuion.
		// Ref:[SVGF]
		vec4 w;
		{
			// Value based weight.
			// Lower value tolerance for the neighbors further apart. Prevents overbluring sharp value transitions.
			// Ref: [Dammertz2010]
			const vec4 errorOffset = vec4(0.005f);
			vec4 valueSigmaDistCoef = vec4(1.0 / length(pixelOffset / invScreenSize));
			vec4 e_x = -abs(value - iValue) / (valueSigmaDistCoef * valueSigma * stdDeviation + errorOffset);
			vec4 w_x = exp(e_x);

			// Normal based weight.
			float w_n = pow(max(0, dot(normal, iNormal)), normalSigma);

			// Depth based weight.
			float w_d;
			{
				float depthFloatPrecision = FloatPrecision(max(depth, iDepth), 5, 10);
				float depthThreshold = DepthThreshold(depth, ddxy, pixelOffset);
				float depthTolerance = depthSigma * depthThreshold + depthFloatPrecision;
				float delta = abs(depth - iDepth);
				delta = max(0, delta - depthFloatPrecision); // Avoid distinguising initial values up to the float precision. Gets rid of banding due to low depth precision format.
				w_d = exp(-delta / depthTolerance);
				// Scale down contributions for samples beyond tolerance, but completely disable contribution for samples too far away.
				const float depthWeightCutoff = 0.2;
				w_d *= float(w_d >= depthWeightCutoff);
			}

			// Filter kernel weight.
			float w_h = Kernel[row][col];

			// Final weight.
			w = w_h * w_n * w_x * w_d;
		}

		weightedValueSum += w * iValue;
		weightSum += w;
	}
}

void main()
{
	uint level = object.level;

	vec4 gbuffer0Data = texture(gbuffer0, screenCoord);

	vec4 value = texture(inputImage, screenCoord);
	vec4 filteredValue = value;

	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 normal = normalize(DecodeNormalViewSpace(gbuffer0Data));

	vec2 ddxy = vec2(dFdx(depth), dFdy(depth));

	vec2 screenSize = vec2(textureSize(inputImage, 0));
	vec2 invScreenSize = vec2(1.0) / screenSize;

	vec2 kernelStep = vec2(exp2(float(level - 1)));

	float w = Kernel[Radius][Radius];

	vec4 weightSum = vec4(w);
	vec4 weightedValueSum = weightSum * value;

	vec4 variance = texture(varianceImage, screenCoord);
	vec4 stdDeviation = sqrt(variance);

	const float minVarianceToDenoise = 0;
	float composeVariance = 0.2126 * variance.x + 0.7152 * variance.y + 0.0722 * variance.z;

	// composeVariance = max(variance.x, variance.y);
	// composeVariance = max(composeVariance, variance.z);
	// composeVariance = max(composeVariance, variance.w);
	// variance = vec4(composeVariance);

	if (composeVariance >= minVarianceToDenoise)
	{
		for (uint r = 0; r < Width; r++)
		{
			for (uint c = 0; c < Width; c++)
			{
				if (r != Radius || c != Radius)
				{
					AddFilterContribution(
						weightedValueSum, 
						weightSum, 
						value, 
						stdDeviation,
						depth, 
						normal, 
						ddxy,
						r, 
						c,
						screenCoord,
						kernelStep,
						invScreenSize);
				}
			}
		}
	}

	filteredValue = weightedValueSum / weightSum;
	finalImage = filteredValue;
}