#ifndef _RTAO_DEFINE_H_
#define _RTAO_DEFINE_H_

#define BINDING_GBUFFER_NORMAL 0
#define BINDING_GBUFFER_POSITION 1
#define BINDING_VELOCITY 2
#define BINDING_AS 3
#define BDINING_UNIFORM 4
#define BINDING_LOCAL_MEAN_VARIANCE_INPUT 5
#define BINDING_LOCAL_MEAN_VARIANCE_OUTPUT 6
#define BINDING_TEMPORAL_SQAREDMEAN_VARIANCE 7
#define BINDING_PREV 8
#define BINDING_FINAL 9
#define BINDING_PREV_NORMAL_DEPTH 10
#define BINDING_CUR_NORMAL_DEPTH 11
#define BINDING_CUR 12
#define BINDING_ATROUS 13
#define BINDING_COMPOSED 14

bool IsWithinBounds(ivec2 pos, ivec2 size)
{
	return pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y;
}

vec4 GetBilinearWeights(in vec2 targetOffset)
{
	vec4 bilinearWeights = vec4
	(
			(1 - targetOffset.x) * (1 - targetOffset.y),
			targetOffset.x * (1 - targetOffset.y),
			(1 - targetOffset.x) * targetOffset.y,
			targetOffset.x * targetOffset.y
	);
	return bilinearWeights;
}

void ComputeWeights(vec2 targetOffset, ivec2 size, ivec2 samplePos[4], in out vec4 weights)
{
	vec4 isWithinBounds = vec4(
		IsWithinBounds(samplePos[0], size),
		IsWithinBounds(samplePos[1], size),
		IsWithinBounds(samplePos[2], size),
		IsWithinBounds(samplePos[3], size));

	weights = GetBilinearWeights(targetOffset);
	weights *= isWithinBounds;
}

#define RTAO_GROUP_SIZE 32
layout(local_size_x = RTAO_GROUP_SIZE, local_size_y = RTAO_GROUP_SIZE) in;

#endif