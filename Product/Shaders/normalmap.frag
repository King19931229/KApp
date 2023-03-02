layout(early_fragment_tests) in;

layout(location = 0) in vec2 inUV;

layout(location = 1) in vec4 inWorldPos;

layout(location = 2) in vec4 inViewPos;

layout(location = 3) in vec4 inTangentLightDir;
layout(location = 4) in vec4 inTangentViewDir;
layout(location = 5) in vec4 inTangentPos;

layout(location = 0) out vec4 outColor;

#include "public.h"
#include "shadow/cascaded/static_mask.h"

layout(binding = BINDING_DIFFUSE) uniform sampler2D diffuseSampler;
layout(binding = BINDING_NORMAL) uniform sampler2D normalSampler;

layout(binding = BINDING_FRAGMENT_SHADING)
uniform Parameter
{
 	float heightScale;
	float parallaxBias;
	float numLayers;
	int mappingMode;
}parameter;

vec2 parallax_uv(vec2 uv, vec3 view_dir, int type)
{
	if (type == 2) {
		// Parallax mapping
		float depth = 1.0 - textureLod(normalSampler, uv, 0.0).a;
		vec2 p = view_dir.xy * (depth * (parameter.heightScale * 0.5) + parameter.parallaxBias) / view_dir.z;
		return uv - p;  
	} else {
		float layer_depth = 1.0 / parameter.numLayers;
		float cur_layer_depth = 0.0;
		vec2 delta_uv = view_dir.xy * parameter.heightScale / (view_dir.z * parameter.numLayers);
		vec2 cur_uv = uv;

		float depth_from_tex = 1.0 - textureLod(normalSampler, cur_uv, 0.0).a;

		for (int i = 0; i < 32; i++) {
			cur_layer_depth += layer_depth;
			cur_uv -= delta_uv;
			depth_from_tex = 1.0 - textureLod(normalSampler, cur_uv, 0.0).a;
			if (depth_from_tex < cur_layer_depth) {
				break;
			}
		}

		if (type == 3) {
			// Steep parallax mapping
			return cur_uv;
		} else {
			// Parallax occlusion mapping
			vec2 prev_uv = cur_uv + delta_uv;
			float next = depth_from_tex - cur_layer_depth;
			float prev = 1.0 - textureLod(normalSampler, prev_uv, 0.0).a - cur_layer_depth + layer_depth;
			float weight = next / (next - prev);
			return mix(cur_uv, prev_uv, weight);
		}
	}
}

vec2 parallaxMapping(vec2 uv, vec3 viewDir) 
{
	float height = 1.0 - textureLod(normalSampler, uv, 0.0).a;
	vec2 p = viewDir.xy * (height * (parameter.heightScale * 0.5) + parameter.parallaxBias) / viewDir.z;
	return uv - p;  
}

vec2 steepParallaxMapping(vec2 uv, vec3 viewDir) 
{
	float layerDepth = 1.0 / parameter.numLayers;
	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * parameter.heightScale / (viewDir.z * parameter.numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(normalSampler, currUV, 0.0).a;
	for (int i = 0; i < parameter.numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(normalSampler, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	return currUV;
}

vec2 parallaxOcclusionMapping(vec2 uv, vec3 viewDir) 
{
	float layerDepth = 1.0 / parameter.numLayers;
	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * parameter.heightScale / (viewDir.z * parameter.numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(normalSampler, currUV, 0.0).a;
	for (int i = 0; i < parameter.numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(normalSampler, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	vec2 prevUV = currUV + deltaUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureLod(normalSampler, prevUV, 0.0).a - currLayerDepth + layerDepth;
	return mix(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}

void main()
{
	vec3 V = inTangentViewDir.xyz;
	vec2 uv = inUV;
	
	switch(parameter.mappingMode)
	{
		case 2:
			uv = parallaxMapping(inUV, V);
			break;
		case 3:
			uv = steepParallaxMapping(inUV, V);
			break;
		case 4:
			uv = parallaxOcclusionMapping(inUV, V);
			break;
	}

	// Discard fragments at texture border
	/*
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
	{
		discard;
	}
	*/

	vec3 N;

	if (parameter.mappingMode == 0)
	{
		N = vec3(0,1,0);
	}
	else
	{
		N = normalize(textureLod(normalSampler, uv, 0.0).rgb * 2.0 - 1.0);
	}

	vec3 L = -inTangentLightDir.xyz;
	//vec3 R = reflect(-L, N);
	//vec3 H = normalize(L + V);

	float NDotL = max(dot(N, L), 0.0);
	float ambient = 0.5f;

	outColor = texture(diffuseSampler, inUV) * (NDotL + ambient);
	
	outColor *= CalcStaticCSM(inWorldPos.xyz);
}