#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rayinput.h"

hitAttributeEXT vec2 attribs;

// clang-format off
layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) readonly buffer Indices {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) readonly buffer Materials { RayTraceMaterial m[]; };
layout(binding = RAYTRACE_BINDING_AS) uniform accelerationStructureEXT topLevelAS;
layout(binding = RAYTRACE_BINDING_SCENE, scalar) readonly buffer SceneDesc_ { SceneDesc i[]; } sceneDesc;
layout(binding = RAYTRACE_BINDING_TEXTURES) uniform sampler2D texturesMap[]; // all textures
// clang-format on

void main()
{
	// Object data
	SceneDesc  objResource = sceneDesc.i[gl_InstanceCustomIndexEXT];
	Indices    indices     = Indices(objResource.indexAddress);
	Vertices   vertices    = Vertices(objResource.vertexAddress);
	Materials  materials   = Materials(objResource.materialAddress);

	// Material per instance, so the index is always 0
	RayTraceMaterial mat = materials.m[0];

	// Indices of the triangle
	ivec3 ind = indices.i[gl_PrimitiveID];

	// Vertex of the triangle
	Vertex v0 = vertices.v[ind.x];
	Vertex v1 = vertices.v[ind.y];
	Vertex v2 = vertices.v[ind.z];

	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	// Computing the normal at hit position
	vec3 normal = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
	// Transforming the normal to world space
	normal = normalize(vec3(objResource.transfoIT * vec4(normal, 0.0)));

	const vec2 uv0       = v0.texCoord;
	const vec2 uv1       = v1.texCoord;
	const vec2 uv2       = v2.texCoord;
	const vec2 texcoord  = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

	// Computing the coordinates of the hit position
	vec3 worldPos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
	// Transforming the position to world space
	worldPos = vec3(objResource.transfo * vec4(worldPos, 1.0));

	// Vector toward the light
	vec3 L = normalize(vec3(1) - vec3(0));

	if(mat.diffuseTex > -1)
  	{
 		int txtId = mat.diffuseTex;
 		prd.hitValue = texture(texturesMap[nonuniformEXT(txtId)], texcoord).xyz;
	}
	else
	{
		prd.hitValue = normal;
	}
	prd.attenuation = vec3(1,1,1);//vec3(dot(L, normal));
	prd.done = 1;
}