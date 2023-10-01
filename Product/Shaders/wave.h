#ifndef _WAVE_H_
#define	_WAVE_H_

#define WAVE_OPERATION_SUPPORT 1

#if WAVE_OPERATION_SUPPORT

#extension GL_ARB_shader_atomic_counters : require
#extension GL_ARB_shader_ballot : require
#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_KHR_shader_subgroup_ballot : require

#define WAVE_INTERLOCK_ADD(dest, value, originalValue)\
{\
	uint numToAdd = subgroupAdd(value);\
	if (subgroupElect())\
	{\
		originalValue = atomicAdd(dest, numToAdd);\
	}\
	originalValue = subgroupBroadcastFirst(originalValue) + subgroupExclusiveAdd(value);\
}

#define WAVE_INTERLOCK_ADD_ONLY(dest, value)\
{\
	uint numToAdd = subgroupAdd(value);\
	if (subgroupElect())\
	{\
		atomicAdd(dest, numToAdd);\
	}\
}

#else

#define WAVE_INTERLOCK_ADD(dest, value, originalValue) originalValue = atomicAdd(dest, value);
#define WAVE_INTERLOCK_ADD_ONLY(dest, value) atomicAdd(dest, value);

#endif

#endif