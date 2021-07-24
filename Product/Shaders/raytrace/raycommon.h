#ifndef RAY_COMMON_H
#define RAY_COMMON_H

#define RAYTRACE_BINDING_AS 0
#define RAYTRACE_BINDING_IMAGE 1
#define RAYTRACE_BINDING_CAMERA 2
#define RAYTRACE_BINDING_SCENE 3
#define RAYTRACE_BINDING_TEXTURES 4

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