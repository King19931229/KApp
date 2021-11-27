#ifndef VOXEL_COMMON_H
#define VOXEL_COMMON_H

#define VOXEL_BINDING_ALBEDO BINDING_TEXTURE9
#define VOXEL_BINDING_NORMAL BINDING_TEXTURE10
#define VOXEL_BINDING_EMISSION BINDING_TEXTURE11
#define VOXEL_BINDING_STATIC_FLAG BINDING_TEXTURE12
#define VOXEL_BINDING_DIFFUSE_MAP BINDING_TEXTURE13
#define VOXEL_BINDING_OPACITY_MAP BINDING_TEXTURE14
#define VOXEL_BINDING_EMISSION_MAP BINDING_TEXTURE15
#define VOXEL_BINDING_RADIANCE BINDING_TEXTURE16
#define VOXEL_BINDING_COMPOSITE BINDING_TEXTURE17
#define VOXEL_BINDING_TEXMIPMAP_IN BINDING_TEXTURE18
#define VOXEL_BINDING_TEXMIPMAP_OUT BINDING_TEXTURE19

#define GROUP_SIZE 8

const float SQRT_3 = 1.73205080f;
const uint MAX_DIRECTIONAL_LIGHTS = 3;
const uint MAX_POINT_LIGHTS = 6;
const uint MAX_SPOT_LIGHTS = 6;

struct Attenuation
{
	float constant;
	float linear;
	float quadratic;
};

struct Light
{
	float angleInnerCone;
	float angleOuterCone;
	vec3 diffuse;
	vec3 position;
	vec3 direction;
	uint shadowingMethod;
	Attenuation attenuation;
};

#if 0
uniform Light directionalLight[MAX_DIRECTIONAL_LIGHTS];
uniform Light pointLight[MAX_POINT_LIGHTS];
uniform Light spotLight[MAX_SPOT_LIGHTS];
uniform uint lightTypeCount[3];
uniform mat4 lightViewProjection;
uniform sampler2D shadowMap;
#endif

vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)), 
	float((val & 0x0000FF00) >> 8U), 
	float((val & 0x00FF0000) >> 16U), 
	float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U | 
	(uint(val.z) & 0x000000FF) << 16U | 
	(uint(val.y) & 0x000000FF) << 8U | 
	(uint(val.x) & 0x000000FF);
}

vec3 EncodeNormal(vec3 normal)
{
	return normal * 0.5f + vec3(0.5f);
}

vec3 DecodeNormal(vec3 normal)
{
	return normal * 2.0f - vec3(1.0f);
}

#endif