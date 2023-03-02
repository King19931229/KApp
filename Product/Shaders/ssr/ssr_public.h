#ifndef _SSR_PUBLIC_H_
#define _SSR_PUBLIC_H_

vec3 WorldPosToScreenPos(vec3 worldPos)
{
	vec4 screenPos = camera.viewProj * vec4(worldPos, 1.0);
	screenPos /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + vec2(0.5);
	return screenPos.xyz;
}

vec3 ViewPosToScreenPos(vec3 viewPos)
{
	vec4 screenPos = camera.proj * vec4(viewPos, 1.0);
	screenPos /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + vec2(0.5);
	return screenPos.xyz;
}

vec3 ScreenPosToWorldPos(vec3 screenPos)
{
	screenPos.xy = screenPos.xy * 2.0 - vec2(1.0);
	vec4 worldPos = camera.viewInv * camera.projInv * vec4(screenPos, 1.0);
	return worldPos.xyz / worldPos.w;
}

vec3 ScreenPosToViewPos(vec3 screenPos)
{
	screenPos.xy = screenPos.xy * 2.0 - vec2(1.0);
	vec4 viewPos = camera.projInv * vec4(screenPos, 1.0);
	return viewPos.xyz / viewPos.w;
}

#define SSR_SUPPRESS_OVER_BRGHT 1
#define SSR_OVERRIDE_ROUGHNESS 0
#define SSR_ROUGHNESS 0.05

#endif