#ifndef RAY_COMMON_H
#define RAY_COMMON_H

#define RAYTRACE_BINDING_AS 0
#define RAYTRACE_BINDING_IMAGE 1
#define RAYTRACE_BINDING_CAMERA 2
#define RAYTRACE_BINDING_SCENE 3
#define RAYTRACE_BINDING_TEXTURES 4
#define RAYTRACE_BINDING_GBUFFER0 5
#define RAYTRACE_BINDING_GBUFFER1 6

struct RayCone
{
	float width; // Called wi in the text
	float spreadAngle; // Called γi in the text
};

struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct SurfaceHit
{
	vec3 normal;
	float surfaceSpreadAngle; // Initialized according to Eq. 32
};

struct Pixel
{
	vec2 t0, t1, t2;
	vec3 p0, p1, p2;
	float wh;
};

struct HitPayload
{
	vec3 hitValue;
	vec3 attenuation;
	vec3 sum;
	vec3 rayOrigin;
	vec3 rayDir;
	int  depth;
	int  done;

	RayCone cone;
};

layout(binding = RAYTRACE_BINDING_AS) uniform accelerationStructureEXT topLevelAS;
layout(binding = RAYTRACE_BINDING_IMAGE, rgba8) uniform image2D image;
layout(binding = RAYTRACE_BINDING_CAMERA) uniform CameraProperties 
{
	mat4 view;
	mat4 proj;
	mat4 viewInv;
	mat4 projInv;
	// near, far, fov, aspect
	vec4 parameters;
} camera;
layout(binding = RAYTRACE_BINDING_TEXTURES) uniform sampler2D texturesMap[]; // all textures

layout(binding = RAYTRACE_BINDING_GBUFFER0) uniform sampler2D gbuffer0;
layout(binding = RAYTRACE_BINDING_GBUFFER1) uniform sampler2D gbuffer1;

#endif