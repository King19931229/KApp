#include "public.h"
#include "voxel/svo/voxel_common.h"

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
} Out;

#include "voxel/voxelzation_public.h"

void main()
{
	int selectedIndex = CalculateAxis();
	mat4 viewProjection = voxel.viewproj[selectedIndex];
	mat4 viewProjectionI = voxel.viewproj_inv[selectedIndex];

	vec2 texCoord[3];
	vec3 normal[3];
	for (int i = 0; i < gl_in.length(); i++)
	{
		texCoord[i] = In[i].texCoord; 
		normal[i] = In[i].normal; 
	}

	//transform vertices to clip space
	vec4 pos[3] = vec4[3]
	(
		viewProjection * gl_in[0].gl_Position,
		viewProjection * gl_in[1].gl_Position,
		viewProjection * gl_in[2].gl_Position
	);

	vec4 trianglePlane;
	trianglePlane.xyz = cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz);
	trianglePlane.xyz = normalize(trianglePlane.xyz);
	trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

	// change winding, otherwise there are artifacts for the back faces.
	if (dot(trianglePlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
	{
		vec4 vertexTemp = pos[2];
		vec2 texCoordTemp = texCoord[2];
		vec3 normalTemp = normal[2];
		
		pos[2] = pos[1];
		texCoord[2] = texCoord[1];
		normal[2] = normal[1];
	
		pos[1] = vertexTemp;
		texCoord[1] = texCoordTemp;
		normal[1] = normalTemp;
	}

	vec2 halfPixel = vec2(1.0f / volumeDimension);

	if(trianglePlane.z == 0.0f) return;

	// expanded aabb for triangle
	Out.triangleAABB = AxisAlignedBoundingBox(pos, halfPixel);
#if 0
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
#else
	// https://github.com/otaku690/SparseVoxelOctree/blob/master/WIN/SVO/shader/voxelize.geom.glsl
	float pl = 1.4142135637309 / volumeDimension;

	vec3 e0 = vec3(pos[1].xy - pos[0].xy, 0);
	vec3 e1 = vec3(pos[2].xy - pos[1].xy, 0);
	vec3 e2 = vec3(pos[0].xy - pos[2].xy, 0);
	vec3 n0 = cross(e0, vec3(0, 0, 1));
	vec3 n1 = cross(e1, vec3(0, 0, 1));
	vec3 n2 = cross(e2, vec3(0, 0, 1));

	// dilate the triangle
	pos[0].xy = pos[0].xy + pl * ((e2.xy / dot(e2.xy, n0.xy)) + (e0.xy / dot(e0.xy, n2.xy)));
	pos[1].xy = pos[1].xy + pl * ((e0.xy / dot(e0.xy, n1.xy)) + (e1.xy / dot(e1.xy, n0.xy)));
	pos[2].xy = pos[2].xy + pl * ((e1.xy / dot(e1.xy, n2.xy)) + (e2.xy / dot(e2.xy, n1.xy)));
#endif

	for(int i = 0; i < 3; ++i)
	{
		vec4 voxelPos = viewProjectionI * pos[i];
		voxelPos.xyz /= voxelPos.w;
		voxelPos.xyz -= voxel.minpoint_scale.xyz;
		voxelPos *= voxel.minpoint_scale.w;

		gl_Position = pos[i];
		Out.position = pos[i].xyz;
		Out.normal = normal[i];
		Out.texCoord = texCoord[i];
		Out.wsPosition = voxelPos.xyz * volumeDimension;

		EmitVertex();
	}

	EndPrimitive();
}