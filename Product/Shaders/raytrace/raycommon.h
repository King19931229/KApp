#ifndef RAY_COMMON_H
#define RAY_COMMON_H

struct hitPayload
{
	vec3 hitValue;
	int  depth;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

#endif