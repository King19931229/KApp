#include "public.h"
#include "pbr.h"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 uvw;
layout(location = 2) in float roughness;

layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform samplerCube samplerEnvMap;

void main()
{
	vec3 N = normalize(uvw);

	vec3 R = N;
	vec3 V = R;

	const uint SAMPLE_COUNT = 64;
	float totalWeight = 0.0;   
	vec3 prefilteredColor = vec3(0.0);     
	for(uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		vec3 H  = ImportanceSampleGGX(Xi, N, roughness).xyz;
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if(NdotL > 0.0)
		{
			vec3 color = TextureCubeFixed(samplerEnvMap, L).rgb;
			prefilteredColor += color * NdotL;
			totalWeight      += NdotL;
		}
	}
	prefilteredColor = prefilteredColor / totalWeight;

	outColor = vec4(prefilteredColor, 1.0);
}