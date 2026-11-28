#ifndef _DOF_BRACKETING_H_
#define _DOF_BRACKETING_H_

#include "dof/circular.h"

// 各编码核数组 (.z, .w) 的离线最大值 B = max(G')；取自 circular.h 并由自动测试校验。
const vec2 Kernel0BracketedMax_1 = vec2(0.104884, 0.101559);
const vec2 Kernel0BracketedMax_2 = vec2(0.111774, 0.107244);
const vec2 Kernel1BracketedMax_2 = vec2(0.133679, 0.106584);

vec2 DofEncodeBracketedHDR(vec2 h, float sum, vec2 kernelMax)
{
	// 非负输入契约下 sum <= 0 仅应为零；使用 !(sum > 0.0) 同时防范 NaN。
	if (!(sum > 0.0)) return vec2(0.0);
	return (h / sum) / kernelMax;
}

vec4 DofEncodeBracketedHDR(vec4 h, float sum, vec4 kernelMax)
{
	if (!(sum > 0.0)) return vec4(0.0);
	return (h / sum) / kernelMax;
}

vec2 DofDecodeBracketedHDR(vec2 encoded, float mean, vec4 brackets, vec2 kernelMax)
{
	// circular.h 的 Brackets 依次保存 (O_R, S_R, O_I, S_I)，ColorMean = M / KERNEL_COUNT。
	float sum = mean * float(KERNEL_COUNT);
	vec2 offset = brackets.xz;
	vec2 scale = brackets.yw;
	return sum * (encoded * kernelMax * scale + offset);
}

#endif
