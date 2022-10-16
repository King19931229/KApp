#ifndef _VOXELZATION_PUBLIC_H_
#define _VOXELZATION_PUBLIC_H_

int CalculateAxis()
{
	vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 faceNormal = cross(p1, p2);

	float nDX = abs(faceNormal.x);
	float nDY = abs(faceNormal.y);
	float nDZ = abs(faceNormal.z);

	if( nDX > nDY && nDX > nDZ )
	{
		return 0;
	}
	else if( nDY > nDX && nDY > nDZ  )
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

vec4 AxisAlignedBoundingBox(vec4 pos[3], vec2 pixelDiagonal)
{
	vec4 aabb;

	aabb.xy = min(pos[2].xy, min(pos[1].xy, pos[0].xy));
	aabb.zw = max(pos[2].xy, max(pos[1].xy, pos[0].xy));

	// enlarge by half-pixel
	aabb.xy -= pixelDiagonal;
	aabb.zw += pixelDiagonal;

	return aabb;
}

bool VoxelInFrustum(vec3 center, vec3 extent, in vec4 frustumPlanes[6])
{
	vec4 plane;

	for(int i = 0; i < 6; i++)
	{
		plane = frustumPlanes[i];
		float d = dot(extent, abs(plane.xyz));
		float r = dot(center, plane.xyz) + plane.w;

		if(d + r > 0.0f == false)
		{
			return false;
		}
	}

	return true;
}

#endif