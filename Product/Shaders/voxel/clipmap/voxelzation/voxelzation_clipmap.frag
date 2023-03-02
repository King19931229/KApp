#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"

#extension GL_ARB_shader_image_load_store : require
#extension GL_EXT_shader_image_load_formatted : require

layout(location = 0) in GeometryOut
{
	vec3 wsPosition;
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	flat vec4 triangleAABB;
	flat vec3[3] trianglePosW;
} In;

layout (location = 0) out vec4 fragColor;
/*layout (pixel_center_integer)*/ in vec4 gl_FragCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D diffuseMap;
layout(binding = BINDING_TEXTURE1) uniform sampler2D opacityMap;
layout(binding = BINDING_TEXTURE2) uniform sampler2D emissiveMap;

layout(binding = BINDING_TEXTURE3, r32ui) uniform volatile coherent uimage3D voxelAlbedo;
layout(binding = BINDING_TEXTURE4, r32ui) uniform volatile coherent uimage3D voxelNormal;
layout(binding = BINDING_TEXTURE5, r32ui) uniform volatile coherent uimage3D voxelEmission;
layout(binding = BINDING_TEXTURE6, rgba8) uniform writeonly image3D voxelVisibility;
layout(binding = BINDING_TEXTURE7, r8) uniform image3D staticVoxelFlag;

#if 0
uniform struct Material
{
	vec3 diffuse;
	vec3 emissive;
} material;
#endif

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
	uint level;
} object;

#define IMAGE_ATOMIC_RGBA_AVG_DECL(grid)\
void imageAtomicRGBA8Avg__##grid(ivec3 coords, vec4 value)\
{\
	value.rgb *= 255.0;\
	uint newVal = convVec4ToRGBA8(value);\
	uint prevStoredVal = 0;\
	uint curStoredVal;\
	uint numIterations = 0;\
\
	while((curStoredVal = imageAtomicCompSwap(grid, coords, prevStoredVal, newVal))\
			!= prevStoredVal\
			&& numIterations < 255)\
	{\
		prevStoredVal = curStoredVal;\
		vec4 rval = convRGBA8ToVec4(curStoredVal);\
		rval.rgb = (rval.rgb * rval.a);\
		vec4 curValF = rval + value;\
		curValF.rgb /= curValF.a;\
		newVal = convVec4ToRGBA8(curValF);\
\
		++numIterations;\
	}\
}

IMAGE_ATOMIC_RGBA_AVG_DECL(voxelAlbedo);
IMAGE_ATOMIC_RGBA_AVG_DECL(voxelNormal);
IMAGE_ATOMIC_RGBA_AVG_DECL(voxelEmission);

#define IMAGE_ATOMIC_RGBA_AVG_CALL(grid, coords, value) imageAtomicRGBA8Avg__##grid(coords, value)

void main()
{
	if( In.position.x < In.triangleAABB.x || In.position.y < In.triangleAABB.y || 
		In.position.x > In.triangleAABB.z || In.position.y > In.triangleAABB.w )
	{
		discard;
	}

	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[object.level].w;

	if (!IntersectsTriangle(In.wsPosition, voxelSize, In.trianglePosW))
		discard;

	if (!InsideUpdateRegion(In.wsPosition, object.level))
		discard;

	// writing coords position
	ivec3 position = WorldPositionToImageCoord(In.wsPosition, object.level, 0);

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
		// average normal per fragments sorrounding the voxel volume
		IMAGE_ATOMIC_RGBA_AVG_CALL(voxelNormal, position, normal);
		// average albedo per fragments sorrounding the voxel volume
		IMAGE_ATOMIC_RGBA_AVG_CALL(voxelAlbedo, position, albedo);
		// average emission per fragments sorrounding the voxel volume
		IMAGE_ATOMIC_RGBA_AVG_CALL(voxelEmission, position, emissive);

		// store the visibility into the alpha channel
		for (uint face = 0; face < 6; ++face)
		{
			imageStore(voxelVisibility, position + GetFaceOffset(face), vec4(1));
		}

		// doing a static flagging pass for static geometry voxelization
		if (storeVisibility == 1)
		{
			imageStore(staticVoxelFlag, position, vec4(1.0));
		}
	}
}