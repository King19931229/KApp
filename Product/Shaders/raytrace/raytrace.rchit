#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rayinput.h"
#include "raycone.h"

hitAttributeEXT vec2 attribs;

// clang-format off
layout(binding = RAYTRACE_BINDING_SCENE, scalar) readonly buffer SceneDesc_ { SceneDesc i[]; } sceneDesc;
layout(location = 0) rayPayloadInEXT HitPayload prd;
layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) readonly buffer Indices {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) readonly buffer Materials { RayTraceMaterial m[]; };
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
	const vec3 p0        = vec3(objResource.transfo * vec4(v0.pos, 1.0));
	const vec3 p1        = vec3(objResource.transfo * vec4(v1.pos, 1.0));
	const vec3 p2        = vec3(objResource.transfo * vec4(v2.pos, 1.0));
	const vec3 worldPos  = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;

	// Using 0 since no curvature measure at second hit
	prd.cone = Propagate(prd.cone, 0, distance(worldPos, prd.rayOrigin));

	float lambda = 0;
	if(mat.diffuseTex > -1)
	{
		int txtId = mat.diffuseTex;
		vec2 texSize = vec2(textureSize(texturesMap[nonuniformEXT(txtId)], 0));
		Pixel pixel;
		pixel.t0 = uv0; pixel.t1 = uv1; pixel.t2 = uv2;
		pixel.p0 = p0; pixel.p1 = p1; pixel.p2 = p2;
		pixel.wh = texSize.x * texSize.y;
		float numMipmap = float(textureQueryLevels(texturesMap[nonuniformEXT(txtId)]));

		lambda = ComputeTextureLOD(prd.rayDir, normal, prd.cone, pixel);
		// lambda = 0;
		prd.hitValue = textureLod(texturesMap[nonuniformEXT(txtId)], texcoord, lambda).xyz;
		lambda /= numMipmap;
	}
	else
	{
		float numMipmap = 3;
		vec2 texSize = vec2(pow(2, numMipmap));
		/*
		Pixel pixel;
		pixel.t0 = uv0; pixel.t1 = uv1; pixel.t2 = uv2;
		pixel.p0 = p0; pixel.p1 = p1; pixel.p2 = p2;
		pixel.wh = texSize.x * texSize.y;
		lambda = ComputeTextureLOD(prd.rayDir, normal, prd.cone, pixel);
		lambda /= numMipmap;
		*/
		prd.hitValue = normal;
	}

	// prd.hitValue = vec3(lambda);
	prd.rayOrigin = prd.rayOrigin + prd.rayDir * gl_HitTEXT;
	prd.rayDir = reflect(prd.rayDir, normal);
	prd.rayOrigin += prd.rayDir * 0.001;
	prd.done = 0;
}