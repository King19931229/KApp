#include "public.h"
#include "voxel_common.h"

#extension GL_ARB_shader_image_load_store : require
#extension GL_EXT_shader_image_load_formatted : require

layout(location = 0) in GeometryOut
{
	vec3 wsPosition;
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	flat vec4 triangleAABB;
} In;

layout (location = 0) out vec4 fragColor;
/*layout (pixel_center_integer)*/ in vec4 gl_FragCoord;

layout(binding = VOXEL_BINDING_STATIC_FLAG, r8) uniform image3D staticVoxelFlag;
layout(binding = VOXEL_BINDING_DIFFUSE_MAP) uniform sampler2D diffuseMap;
layout(binding = VOXEL_BINDING_OPACITY_MAP) uniform sampler2D opacityMap;
layout(binding = VOXEL_BINDING_EMISSION_MAP) uniform sampler2D emissiveMap;

layout(binding = VOXEL_BINDING_COUNTER) buffer uuCounter { uint uCounter; };
layout(binding = VOXEL_BINDING_FRAGMENTLIST) writeonly buffer uuFragmentList { uvec2 uFragmentList[]; };
layout(binding = VOXEL_BINDING_COUNTONLY) readonly buffer uuCountOnly { uint uCountOnly; };

#if 0
uniform struct Material
{
	vec3 diffuse;
	vec3 emissive;
} material;
#endif

void main()
{
	if( In.position.x < In.triangleAABB.x || In.position.y < In.triangleAABB.y || 
		In.position.x > In.triangleAABB.z || In.position.y > In.triangleAABB.w )
	{
		discard;
	}

	// writing coords position
	ivec3 position = ivec3(In.wsPosition);
	// fragment albedo
	vec4 albedo = texture(diffuseMap, In.texCoord.xy);
#if 0
	float opacity = min(albedo.a, texture(opacityMap, In.texCoord.xy).r);
#else
	float opacity = 1.0;
#endif

	// alpha cutoff
	if(opacity > 0.0f)
	{
		// albedo is in srgb space, bring back to linear
		albedo.rgb = albedo.rgb;// * material.diffuse;
		// premultiplied alpha
		albedo.rgb *= opacity;
		albedo.a = 1.0f;
		// emission value
		vec4 emissive = vec4(0);//texture(emissiveMap, In.texCoord.xy);
		emissive.rgb = emissive.rgb;// * material.emissive;
		emissive.a = 1.0f;
		// bring normal to 0-1 range
		vec4 normal = vec4(EncodeNormal(normalize(In.normal)), 1.0f);

		uint ucolor = packUnorm4x8(albedo);
		uint cur = atomicAdd(uCounter, 1u);
		// set fragment list
		if (uCountOnly == 0)
		{
			uvec3 uvoxel_pos = clamp(uvec3(position * volumeDimension), uvec3(0u), uvec3(volumeDimension - 1u));
			 // only have the last 8 bits of uvoxel_pos.z
			uFragmentList[cur].x = uvoxel_pos.x | (uvoxel_pos.y << 12u) | ((uvoxel_pos.z & 0xffu) << 24u);
			uFragmentList[cur].y = ((uvoxel_pos.z >> 8u) << 28u) | (ucolor & 0x00ffffffu);
		}
	}
}