#ifdef VIRTUAL_GEOMETRY
#include "public.h"
#else
#include "public.h"
mat4 prevWorldToClip = camera.prevViewProj;
mat4 worldToClip = camera.viewProj;
#endif

#ifdef GPU_SCENE

#include "gpuscene/gpuscene_define.h"

layout(std430, binding = BINDING_MATERIAL_PARAMETER) readonly buffer MaterialParameterPackBuffer { float MaterialParameterData[]; };
layout(std430, binding = BINDING_MATERIAL_TEXTURE_BINDING) readonly buffer MaterialTextureBindingPackBuffer { MaterialTextureBindingStruct MaterialTextureBinding[]; };
layout(std430, binding = BINDING_MATERIAL_INDEX) readonly buffer MaterialIndexPackBuffer { uint MaterialIndex[]; };

// TODO 重构GPUScene TextureIndex
layout(binding = BINDING_TEXTURE0) uniform sampler2DArray textureArraySampler0;
layout(binding = BINDING_TEXTURE1) uniform sampler2DArray textureArraySampler1;
layout(binding = BINDING_TEXTURE2) uniform sampler2DArray textureArraySampler2;
layout(binding = BINDING_TEXTURE3) uniform sampler2DArray textureArraySampler3;
layout(binding = BINDING_TEXTURE4) uniform sampler2DArray textureArraySampler4;
layout(binding = BINDING_TEXTURE5) uniform sampler2DArray textureArraySampler5;

uint materialIndex = MaterialIndex[darwIndex];

vec4 SampleTextureArray(in vec2 texCoord, in uint arrayIndex, in uint sliceIndex)
{
	// 0.125K
	if (arrayIndex == 0)
	{
		return texture(textureArraySampler0, vec3(texCoord, float(sliceIndex)));
	}
	// 0.25K
	if (arrayIndex == 1)
	{
		return texture(textureArraySampler1, vec3(texCoord, float(sliceIndex)));
	}
	// 0.5K
	if (arrayIndex == 2)
	{
		return texture(textureArraySampler2, vec3(texCoord, float(sliceIndex)));
	}
	// 1K
	if (arrayIndex == 3)
	{
		return texture(textureArraySampler3, vec3(texCoord, float(sliceIndex)));
	}
	// 2K
	if (arrayIndex == 4)
	{
		return texture(textureArraySampler4, vec3(texCoord, float(sliceIndex)));
	}
	// 4K
	if (arrayIndex == 5)
	{
		return texture(textureArraySampler5, vec3(texCoord, float(sliceIndex)));
	}
	return vec4(0);
}

vec4 SampleDiffuse(in vec2 texCoord)
{
	return SampleTextureArray(texCoord, MaterialTextureBinding[materialIndex].binding[0], MaterialTextureBinding[materialIndex].slice[0]);
}
vec4 SampleDiffuseLod(in vec2 texCoord, float mip)
{
	// TODO
	return SampleDiffuse(texCoord);
}

vec4 SampleNormal(in vec2 texCoord)
{
	return SampleTextureArray(texCoord, MaterialTextureBinding[materialIndex].binding[1], MaterialTextureBinding[materialIndex].slice[1]);
}

#if PBR_MATERIAL_SPECULAR_GLOSINESS
vec4 SampleSpecularGlosiness(in vec2 texCoord)
{
	return SampleTextureArray(texCoord, MaterialTextureBinding[materialIndex].binding[2], MaterialTextureBinding[materialIndex].slice[2]);
}
#else
vec4 SampleMetalRoughness(in vec2 texCoord)
{
	return SampleTextureArray(texCoord, MaterialTextureBinding[materialIndex].binding[2], MaterialTextureBinding[materialIndex].slice[2]);
}
#endif

vec4 SampleEmissive(in vec2 texCoord)
{
	return SampleTextureArray(texCoord, MaterialTextureBinding[materialIndex].binding[3], MaterialTextureBinding[materialIndex].slice[3]);
}

vec4 SampleAO(in vec2 texCoord)
{
	return SampleTextureArray(texCoord, MaterialTextureBinding[materialIndex].binding[4], MaterialTextureBinding[materialIndex].slice[4]);
}

struct ShadingStruct
{
	vec4 baseColorFactor;
#if PBR_MATERIAL_SPECULAR_GLOSINESS
	vec4 diffuseFactor;
	vec4 specularFactor;
#endif
	vec4 emissiveFactor;
	float metallicFactor;
	float roughnessFactor;
	float alphaMask;
	float alphaMaskCutoff;
} shading;

#else // ifndef GPU_SCENE

#ifdef VIRTUAL_GEOMETRY
#define MATERIAL_TEXTURE_BINDING0 BINDING_VIRTUAL_GEOMETRY_TEXTURE0
#define MATERIAL_TEXTURE_BINDING1 BINDING_VIRTUAL_GEOMETRY_TEXTURE1
#define MATERIAL_TEXTURE_BINDING2 BINDING_VIRTUAL_GEOMETRY_TEXTURE2
#define MATERIAL_TEXTURE_BINDING3 BINDING_VIRTUAL_GEOMETRY_TEXTURE3
#define MATERIAL_TEXTURE_BINDING4 BINDING_VIRTUAL_GEOMETRY_TEXTURE4
#else
#define MATERIAL_TEXTURE_BINDING0 BINDING_TEXTURE0
#define MATERIAL_TEXTURE_BINDING1 BINDING_TEXTURE1
#define MATERIAL_TEXTURE_BINDING2 BINDING_TEXTURE2
#define MATERIAL_TEXTURE_BINDING3 BINDING_TEXTURE3
#define MATERIAL_TEXTURE_BINDING4 BINDING_TEXTURE4
#endif

#if HAS_MATERIAL_TEXTURE0
layout(binding = MATERIAL_TEXTURE_BINDING0) uniform sampler2D diffuseSampler;
vec4 SampleDiffuse(in vec2 texCoord)
{
	return texture(diffuseSampler, texCoord);
}
vec4 SampleDiffuseLod(in vec2 texCoord, float mip)
{
	return textureLod(diffuseSampler, texCoord, mip);
}
#endif

#if HAS_MATERIAL_TEXTURE1
layout(binding = MATERIAL_TEXTURE_BINDING1) uniform sampler2D normalSampler;
vec4 SampleNormal(in vec2 texCoord)
{
	return texture(normalSampler, texCoord);
}
#endif

#if HAS_MATERIAL_TEXTURE2
#if PBR_MATERIAL_SPECULAR_GLOSINESS
layout(binding = MATERIAL_TEXTURE_BINDING2) uniform sampler2D specularGlosinessSampler;
vec4 SampleSpecularGlosiness(in vec2 texCoord)
{
	return texture(specularGlosinessSampler, texCoord);
}
#else
layout(binding = MATERIAL_TEXTURE_BINDING2) uniform sampler2D metalRoughnessSampler;
vec4 SampleMetalRoughness(in vec2 texCoord)
{
	return texture(metalRoughnessSampler, texCoord);
}
#endif
#endif

#if HAS_MATERIAL_TEXTURE3
layout(binding = MATERIAL_TEXTURE_BINDING3) uniform sampler2D emissiveSampler;
vec4 SampleEmissive(in vec2 texCoord)
{
	return texture(emissiveSampler, texCoord);
}
#endif

#if HAS_MATERIAL_TEXTURE4
layout(binding = MATERIAL_TEXTURE_BINDING4) uniform sampler2D aoSampler;
vec4 SampleAO(in vec2 texCoord)
{
	return texture(aoSampler, texCoord);
}
#endif

layout(binding = BINDING_SHADING)
uniform Shading_DYN_UNIFORM
{
	vec4 baseColorFactor;
#if PBR_MATERIAL_SPECULAR_GLOSINESS
	vec4 diffuseFactor;
	vec4 specularFactor;
#endif
	vec4 emissiveFactor;
	float metallicFactor;
	float roughnessFactor;
	float alphaMask;
	float alphaMaskCutoff;
} shading;

#endif // GPU_SCENE

#if VIRTUAL_TEXTURE_INPUT

layout(binding = BINDING_TEXTURE16) uniform sampler2D physicalSampler0;
layout(binding = BINDING_TEXTURE17) uniform sampler2D physicalSampler1;
layout(binding = BINDING_TEXTURE18) uniform sampler2D physicalSampler2;
layout(binding = BINDING_TEXTURE19) uniform sampler2D physicalSampler3;

layout(std430, binding = BINDING_VIRTUAL_TEXTURE_DESCRIPTION) buffer VirtualTextureDescriptionBuffer { uvec4 VirtualTextureDescription[]; };
layout(std430, binding = BINDING_VIRTUAL_TEXTURE_FEEDBACK_RESULT) buffer VirtualTextureFeedbackResultBuffer { vec4 VirtualTextureFeedbackResult[]; };

layout(binding = BINDING_VIRTUAL_TEXTURE_BINDING)
uniform VirtualTextureBinding_DYN_UNIFORM
{
	uvec4 binding[4]; 
} virtual_binding;

vec4 SampleFromVirtualTexture(uint binding, vec2 texCoord, vec4 pageInfo)
{
#ifndef VIRTUAL_TEXTURE_FEEDBACK_PASS
	if (pageInfo.w != 1.0)
	{
		float pageX = floor(pageInfo.x * 255.0 + 0.5);
		float pageY = floor(pageInfo.y * 255.0 + 0.5);
		float pageMip = floor(pageInfo.z * 255.0 + 0.5);
		float virutalMip = floor(pageInfo.w * 255.0 + 0.5);

		uint virtualID = virtual_binding.binding[binding / 4][binding & 3];
		float maxVirtualMip = VirtualTextureDescription[virtualID].y;

		float tileSize = float(virtual_texture_constant.description.x);
		float paddingSize = float(virtual_texture_constant.description.y);

		vec2 texCoordInTile = fract(texCoord * exp2(maxVirtualMip - virutalMip));

		vec2 texelPos = paddingSize + (tileSize + paddingSize * 2) * vec2(pageX, pageY) + tileSize * texCoordInTile;
		vec2 texCoordInPhysical = texelPos / textureSize(physicalSampler0, int(pageMip));

		return textureLod(physicalSampler0, texCoordInPhysical, pageMip);
	}
	else
	{
		return vec4(0);
	}
#else
	return vec4(0);
#endif
}

float ComputeVirtualMipLevel(vec2 texCoord, uint binding, float mipBias)
{
	uint virtualID = virtual_binding.binding[binding / 4][binding & 3];

	float maxMipLevel = float(VirtualTextureDescription[virtualID].y);
	vec2 textureSize = vec2(float(VirtualTextureDescription[virtualID].w));

	vec2 uv = texCoord * textureSize;
	vec2 dx = dFdx(uv);
	vec2 dy = dFdy(uv);

	return clamp((0.5 * log2(max(dot(dx, dx), dot(dy, dy))) + 0.5 + mipBias), 0, maxMipLevel);
}

#else

vec4 SampleFromVirtualTexture(vec2 texCoord, vec4 pageInfo)
{
	return vec4(0);
}

#endif

#if VIRTUAL_TEXTURE_INPUT

#if defined(VIRTUAL_TEXTURE_FEEDBACK_PASS)

layout(binding = BINDING_VIRTUAL_TEXTURE_FEEDBACK_TARGET)
uniform VirtualTextureFeedback_DYN_UNIFORM
{
	uvec4 miscs;
	uvec4 miscs2;
} virtual_feedback;

void WriteVirtualFeedback(vec2 texCoord, uint binding)
{
	uint virtualTargetBinding = virtual_feedback.miscs.x;
	float feedbackRatio = float(virtual_texture_constant.description2.x);
	if (virtualTargetBinding == binding)
	{
		uint virtualID = virtual_feedback.miscs.y;
		vec2 tableSize = vec2(virtual_feedback.miscs.zw);

		uint mipBias = virtual_feedback.miscs2.x;
		uint maxMip = virtual_feedback.miscs2.y;
		vec2 textureSize = vec2(virtual_feedback.miscs2.zw);

		vec2 page = floor(texCoord * tableSize);

		vec2 uv = texCoord * textureSize;
		vec2 dx = dFdx(uv) / feedbackRatio;
		vec2 dy = dFdy(uv) / feedbackRatio;
		uint mip = clamp(uint(0.5 * log2(max(dot(dx, dx), dot(dy, dy))) + 0.5 + mipBias), 0, maxMip);

		FeedbackRT = vec4(page / 255.0, mip / 255.0, virtualID / 255.0);
	}
}

#elif defined(BASE_PASS)

layout(binding = BINDING_VIRTUAL_TEXTURE_FEEDBACK_TARGET)
uniform VirtualTextureFeedback_DYN_UNIFORM
{
	uvec4 miscs;
	uvec4 miscs2;
} virtual_feedback;

in vec4 gl_FragCoord;
void WriteVirtualFeedback(vec2 uv, uint binding)
{
	uint virtualTargetBinding = virtual_feedback.miscs.x;
	uint feedbackRatio = virtual_texture_constant.description2.x;
	uint targetX = virtual_texture_constant.description2.z;
	uint targetY = virtual_texture_constant.description2.w;
	targetX = targetY = (feedbackRatio - 1) / 2;
	uvec2 texelPos = uvec2(gl_FragCoord.xy);
	if (virtualTargetBinding == binding)
	{
		uint virtualID = virtual_feedback.miscs.y;
		vec2 tableSize = vec2(virtual_feedback.miscs.zw);

		uint mipBias = virtual_feedback.miscs2.x;
		uint maxMip = virtual_feedback.miscs2.y;
		vec2 textureSize = vec2(virtual_feedback.miscs2.zw);

		vec2 page = floor(texCoord * tableSize);

		vec2 uv = texCoord * textureSize;
		vec2 dx = dFdx(uv);
		vec2 dy = dFdy(uv);
		uint mip = clamp(uint(0.5 * log2(max(dot(dx, dx), dot(dy, dy))) + 0.5 + mipBias), 0, maxMip);

		uint feedbackWidth = virtual_texture_constant.description.z;
		if (texelPos.x % feedbackRatio == targetX && texelPos.y % feedbackRatio == targetY)
		{
			uint writePos = texelPos.x / feedbackRatio + (texelPos.y / feedbackRatio) * feedbackWidth;
			VirtualTextureFeedbackResult[writePos] = vec4(page / 255.0, mip / 255.0, virtualID / 255.0);
		}
	}
}

#else

void WriteVirtualFeedback(vec2 uv, uint binding)
{
}

#endif // VIRTUAL_TEXTURE_FEEDBACK_PASS
#endif // VIRTUAL_TEXTURE_INPUT

#define MANUAL_SRGB 0
#define SRGB_FAST_APPROXIMATION 1

const float MIN_ROUGHNESS = 0.03;
const float EPSILON = 1e-6;

float ConvertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	if (perceivedSpecular < MIN_ROUGHNESS)
	{
		return 0.0;
	}
	float a = MIN_ROUGHNESS;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - MIN_ROUGHNESS) + perceivedSpecular - 2.0 * MIN_ROUGHNESS;
	float c = MIN_ROUGHNESS - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
#ifdef MANUAL_SRGB
	#ifdef SRGB_FAST_APPROXIMATION
	vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
	#else //SRGB_FAST_APPROXIMATION
	vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
	vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
	#endif //SRGB_FAST_APPROXIMATION
	return vec4(linOut,srgbIn.w);
#else //MANUAL_SRGB
	return srgbIn;
#endif //MANUAL_SRGB
}

MaterialPixelParameters ComputeMaterialPixelParameters(
	  vec3 worldPos
	, vec3 prevWorldPos
	, vec3 worldNormal
	, vec2 texCoord
#if TANGENT_BINORMAL_INPUT
	, vec3 worldTangent
	, vec3 worldBinormal
#endif
	)
{
#ifdef GPU_SCENE
	uint materialParameterReadOffset = 0;
#if PBR_MATERIAL_SPECULAR_GLOSINESS
	uint materialParameterSize = 20;
#else
	uint materialParameterSize = 12;
#endif
	shading.baseColorFactor = vec4
	(
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 1],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 2],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 3]
	);
	materialParameterReadOffset += 4;
#if PBR_MATERIAL_SPECULAR_GLOSINESS
	shading.diffuseFactor = vec4
	(
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 1],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 2],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 3]
	);
	materialParameterReadOffset += 4;
	shading.specularFactor = vec4
	(
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 1],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 2],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 3]
	);
	materialParameterReadOffset += 4;
#endif
	shading.emissiveFactor = vec4
	(
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 1],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 2],
		MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset + 3]
	);
	materialParameterReadOffset += 4;

	shading.metallicFactor = MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset];
	materialParameterReadOffset += 1;
	shading.roughnessFactor = MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset];
	materialParameterReadOffset += 1;
	shading.alphaMask = MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset];
	materialParameterReadOffset += 1;
	shading.alphaMaskCutoff = MaterialParameterData[darwIndex * materialParameterSize + materialParameterReadOffset];
	materialParameterReadOffset += 1;
#endif // GPU_SCENE

	MaterialPixelParameters parameters;

	vec4 diffuse = vec4(0.0);
#if HAS_MATERIAL_TEXTURE0
#if HAS_VIRTUAL_MATERIAL_TEXTURE0
	WriteVirtualFeedback(texCoord, 0);
	float minVirtualMip = round(255.0 * SampleDiffuseLod(texCoord, 0).w);
	float virtualMip = max(ComputeVirtualMipLevel(texCoord, 0, 0), minVirtualMip);
	vec4 virtualPageInfo0 = SampleDiffuseLod(texCoord, floor(virtualMip));
	vec4 d0 = SRGBtoLINEAR(SampleFromVirtualTexture(0, texCoord, virtualPageInfo0));
	vec4 virtualPageInfo1 = SampleDiffuseLod(texCoord, ceil(virtualMip));
	vec4 d1 = SRGBtoLINEAR(SampleFromVirtualTexture(0, texCoord, virtualPageInfo1));
	diffuse = mix(d0, d1, fract(virtualMip));
#else
	diffuse = SRGBtoLINEAR(SampleDiffuse(texCoord));
#endif
	parameters.baseColor = diffuse.rgb * shading.baseColorFactor.rgb;
	parameters.opacity = diffuse.a * shading.baseColorFactor.a;
#else
	parameters.baseColor = shading.baseColorFactor.rgb;
	parameters.opacity = 1.0;
#endif

	if (shading.alphaMask > 0 && parameters.opacity <= shading.alphaMaskCutoff)
	{
		discard;
	}

	parameters.position = worldPos;

#if (TANGENT_BINORMAL_INPUT && HAS_MATERIAL_TEXTURE1)
	vec3 normalmap = 2.0 * SampleNormal(texCoord).rgb - vec3(1.0);
	// normalmap.rg = 2.0 * SampleNormal(texCoord).ra - vec2(1.0);
	// normalmap.b = sqrt(1.0 - dot(normalmap.rg, normalmap.rg));
	parameters.normal = normalize(worldTangent * normalmap.r
					  + worldBinormal * normalmap.g
					  + worldNormal * normalmap.b);
#else
	parameters.normal = worldNormal;
#endif

	vec4 prev = prevWorldToClip * vec4(prevWorldPos, 1.0);
	vec4 curr = worldToClip * vec4(worldPos, 1.0);
	vec2 prevUV = 0.5 * (prev.xy / prev.w + vec2(1.0));
	vec2 currUV = 0.5 * (curr.xy / curr.w + vec2(1.0));
	parameters.motion = prevUV - currUV;

#if PBR_MATERIAL_METAL_ROUGHNESS
	parameters.roughness = shading.roughnessFactor;
	parameters.metal = shading.metallicFactor;
	#if HAS_MATERIAL_TEXTURE2
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 metalRoughness = SampleMetalRoughness(texCoord);
	parameters.metal *= metalRoughness.b;
	parameters.roughness *= metalRoughness.g;
	#else
	parameters.metal = clamp(parameters.metal, 0.0, 1.0);
	parameters.roughness = clamp(parameters.roughness, MIN_ROUGHNESS, 1.0);
	#endif
#endif

#if PBR_MATERIAL_SPECULAR_GLOSINESS
	vec3 specular;
	#if HAS_MATERIAL_TEXTURE2
	vec4 specularGlosiness = SampleSpecularGlosiness(texCoord);
	parameters.roughness = 1.0 - specularGlosiness.a;
	specular = SRGBtoLINEAR(specularGlosiness).rgb;
	#else
	parameters.roughness = 0;
	specular = vec3(0);
	#endif

	float maxSpecular = max(max(specular.r, specular.g), specular.b);

	// Convert metallic value from specular glossiness inputs
	float metallic = ConvertMetallic(diffuse.rgb, specular, maxSpecular);

	vec3 baseColorDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - MIN_ROUGHNESS) / max(1 - metallic, EPSILON)) * shading.diffuseFactor.rgb;
	vec3 baseColorSpecularPart = specular - (vec3(MIN_ROUGHNESS) * (1 - metallic) * (1 / max(metallic, EPSILON))) * shading.specularFactor.rgb;
	
	parameters.baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a).rgb;
	parameters.metal = metallic;
#endif

#if HAS_MATERIAL_TEXTURE3
	parameters.emissive = SRGBtoLINEAR(SampleEmissive(texCoord)).rgb;
#else
	parameters.emissive = vec3(0);
#endif

#if HAS_MATERIAL_TEXTURE4
	parameters.ao = SampleAO(texCoord).r;
#else
	parameters.ao = 1.0;
#endif

	return parameters;
}