#ifndef VOXEL_COMMON_H
#define VOXEL_COMMON_H

#define VOXEL_BINDING_ALBEDO BINDING_TEXTURE5
#define VOXEL_BINDING_NORMAL BINDING_TEXTURE6
#define VOXEL_BINDING_EMISSION BINDING_TEXTURE7
#define VOXEL_BINDING_STATIC_FLAG BINDING_TEXTURE8
#define VOXEL_BINDING_DIFFUSE_MAP BINDING_TEXTURE9
#define VOXEL_BINDING_OPACITY_MAP BINDING_TEXTURE10
#define VOXEL_BINDING_EMISSION_MAP BINDING_TEXTURE11

vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)), 
	float((val & 0x0000FF00) >> 8U), 
	float((val & 0x00FF0000) >> 16U), 
	float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U | 
	(uint(val.z) & 0x000000FF) << 16U | 
	(uint(val.y) & 0x000000FF) << 8U | 
	(uint(val.x) & 0x000000FF);
}

#endif