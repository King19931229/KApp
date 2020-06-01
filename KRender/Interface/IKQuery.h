#pragma once
#include "KRender/Interface/IKRenderConfig.h"

struct IKQuery
{
	virtual ~IKQuery() {}

	virtual bool Init(QueryType type) = 0;
	virtual bool UnInit() = 0;

	virtual QueryStatus GetStatus() = 0;

	// 同步等待返回结果
	virtual bool GetResultSync(uint32_t& result) = 0;
	// 如果能够直接获取Query结果 返回值为true否则为false
	virtual bool GetResultAsync(uint32_t& result) = 0;	
	// 返回从开始询问到当前的时间间隔(s)
	virtual float GetElapseTime() = 0;
	// 终止询问
	virtual bool Abort() = 0;
};