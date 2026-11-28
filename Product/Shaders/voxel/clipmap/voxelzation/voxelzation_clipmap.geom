#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	mat4 model;
	uint level;
} object;

layout(location = 0) in Vertex
{
	vec2 texCoord;
	vec3 normal;
} In[3];

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out GeometryOut
{
	vec3 wsPosition;
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	flat vec4 triangleAABB;
	flat vec3[3] trianglePosW;
} Out;

#include "voxel/voxelzation_public.h"

void main()
{
	vec2 texCoord[3];
	vec3 normal[3];
	for (int i = 0; i < 3; i++)
	{
		texCoord[i] = In[i].texCoord; 
		normal[i] = In[i].normal;
	}

	vec3 trianglePosW[3] = vec3[3]
	(
		gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w,
		gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w,
		gl_in[2].gl_Position.xyz / gl_in[2].gl_Position.w
	);

	vec3 flatNormal = cross(trianglePosW[1].xyz - trianglePosW[0].xyz, trianglePosW[2].xyz - trianglePosW[0].xyz);
	if (!ClipmapFinite(flatNormal) || dot(flatNormal, flatNormal) == 0.0)
		return;
	vec3 axisNormal = abs(flatNormal);
	int selectedIndex = axisNormal.x >= max(axisNormal.y, axisNormal.z) ? 0 : (axisNormal.y >= axisNormal.z ? 1 : 2);
	mat4 viewProjection = voxel_clipmap.viewproj[object.level * 3 + selectedIndex];
	mat4 viewProjectionI = voxel_clipmap.viewproj_inv[object.level * 3 + selectedIndex];
	flatNormal = normalize(flatNormal);

	//transform vertices to clip space
	vec4 pos[3] = vec4[3]
	(
		viewProjection * gl_in[0].gl_Position,
		viewProjection * gl_in[1].gl_Position,
		viewProjection * gl_in[2].gl_Position
	);

	vec4 trianglePlane;
	trianglePlane.xyz = cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz);
	if (!ClipmapFinite(trianglePlane.xyz) || dot(trianglePlane.xyz, trianglePlane.xyz) == 0.0)
		return;
	trianglePlane.xyz = normalize(trianglePlane.xyz);
	trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

	// change winding, otherwise there are artifacts for the back faces.
	if (dot(trianglePlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
	{
		vec4 vertexTemp = pos[2];
		vec2 texCoordTemp = texCoord[2];
		vec3 normalTemp = normal[2];
		vec3 posWTemp = trianglePosW[2];

		pos[2] = pos[1];
		texCoord[2] = texCoord[1];
		normal[2] = normal[1];
		trianglePosW[2] = trianglePosW[1];

		pos[1] = vertexTemp;
		texCoord[1] = texCoordTemp;
		normal[1] = normalTemp;
		trianglePosW[1] = posWTemp;
	}

	vec2 halfPixel = vec2(1.0f / volumeDimension);

	if(trianglePlane.z == 0.0f) return;

	// expanded aabb for triangle
	vec4 triangleAABB = AxisAlignedBoundingBox(pos, halfPixel);
	vec4 originalPos[3] = vec4[3](pos[0], pos[1], pos[2]);
	// https://github.com/AdamYuan/SparseVoxelOctree/blob/master/shader/voxelizer.geom
	// calculate the plane through each edge of the triangle
	// in normal form for dilatation of the triangle
	vec3 planes[3];
	planes[0] = cross(pos[0].xyw - pos[2].xyw, pos[2].xyw);
	planes[1] = cross(pos[1].xyw - pos[0].xyw, pos[0].xyw);
	planes[2] = cross(pos[2].xyw - pos[1].xyw, pos[1].xyw);
	planes[0].z -= dot(halfPixel, abs(planes[0].xy));
	planes[1].z -= dot(halfPixel, abs(planes[1].xy));
	planes[2].z -= dot(halfPixel, abs(planes[2].xy));
	// calculate intersection between translated planes
	vec3 intersection[3];
	intersection[0] = cross(planes[0], planes[1]);
	intersection[1] = cross(planes[1], planes[2]);
	intersection[2] = cross(planes[2], planes[0]);
	for (int i = 0; i < 3; ++i)
	{
		if (!ClipmapFinite(intersection[i]) || intersection[i].z == 0.0)
			return;
	}
	intersection[0] /= intersection[0].z;
	intersection[1] /= intersection[1].z;
	intersection[2] /= intersection[2].z;
	// calculate dilated triangle vertices
	float z[3];
	z[0] = -(intersection[0].x * trianglePlane.x + intersection[0].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	z[1] = -(intersection[1].x * trianglePlane.x + intersection[1].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	z[2] = -(intersection[2].x * trianglePlane.x + intersection[2].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	pos[0].xyz = vec3(intersection[0].xy, z[0]);
	pos[1].xyz = vec3(intersection[1].xy, z[1]);
	pos[2].xyz = vec3(intersection[2].xy, z[2]);


	for(int i = 0; i < 3; ++i)
	{
		vec4 voxelPos = viewProjectionI * pos[i];
		voxelPos /= voxelPos.w;

		gl_Position = pos[i];
		Out.position = pos[i].xyz;
		// Out.normal = normal[i];
		Out.normal = flatNormal;
		vec2 e0 = originalPos[1].xy - originalPos[0].xy;
		vec2 e1 = originalPos[2].xy - originalPos[0].xy;
		vec2 delta = pos[i].xy - originalPos[0].xy;
		float area = e0.x * e1.y - e0.y * e1.x;
		float b = (delta.x * e1.y - delta.y * e1.x) / area;
		float c = (e0.x * delta.y - e0.y * delta.x) / area;
		Out.texCoord = texCoord[0] * (1.0 - b - c) + texCoord[1] * b + texCoord[2] * c;
		Out.triangleAABB = triangleAABB;
		Out.wsPosition = voxelPos.xyz;
		Out.trianglePosW = trianglePosW;

		EmitVertex();
	}

	EndPrimitive();
}