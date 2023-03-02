#include "public.h"
#include "voxel/svo/voxel_common.h"
#include "voxel/svo/octree/octree_common.h"

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

layout(binding = BINDING_TEXTURE0) uniform sampler2D diffuseMap;
layout(binding = BINDING_TEXTURE1) uniform sampler2D opacityMap;
layout(binding = BINDING_TEXTURE2) uniform sampler2D emissiveMap;

layout(binding = BINDING_TEXTURE3) buffer uuCounter { uint uCounter; };
layout(binding = BINDING_TEXTURE4) writeonly buffer uuFragmentList { uvec4 uFragmentList[]; };
layout(binding = BINDING_TEXTURE5) readonly buffer uuCountOnly { uint uCountOnly; };
layout(binding = BINDING_TEXTURE6, r8) uniform image3D staticVoxelFlag;

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

	if (storeVisibility == 0)
	{
		bool isStatic = imageLoad(staticVoxelFlag, position).r > 0.0f;
		// force condition so writing is canceled
		if(isStatic) opacity = 0.0f;
	}

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
		uint unormal = packUnorm4x8(normal);
		uint ucolor = packUnorm4x8(albedo);
		uint uemissive = packUnorm4x8(emissive);

		uint cur = atomicAdd(uCounter, 1u);
		// set fragment list
		if (uCountOnly == 0)
		{
			uvec3 uvoxel_pos = clamp(uvec3(position), uvec3(0u), uvec3(volumeDimension - 1u));
			 // only have the last 8 bits of uvoxel_pos.z
			uFragmentList[cur].x = uvoxel_pos.x | (uvoxel_pos.y << 12u) | ((uvoxel_pos.z & 0xffu) << 24u);
			uFragmentList[cur].y = ((uvoxel_pos.z >> 8u) << 28u) | (ucolor & 0x00ffffffu);
			uFragmentList[cur].z = unormal;
			uFragmentList[cur].w = uemissive;
		}

		// doing a static flagging pass for static geometry voxelization
		if (storeVisibility == 1)
		{
			imageStore(staticVoxelFlag, position, vec4(1.0));
		}
	}
}