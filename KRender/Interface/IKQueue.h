#pragma once
#include "KRender/Interface/IKRenderConfig.h"

struct IKSemaphore
{
	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
	virtual bool SetDebugName(const char* name) = 0;
};

struct IKFence
{
	virtual bool Init(bool singaled) = 0;
	virtual bool UnInit() = 0;
	virtual bool SetDebugName(const char* name) = 0;
	virtual bool Wait() = 0;
	virtual bool Reset() = 0;
};

struct IKQueue
{
	virtual bool Init(QueueCategory category, uint32_t queueIndex) = 0;
	virtual bool UnInit() = 0;
	virtual QueueCategory GetCategory() const = 0;
	virtual uint32_t GetIndex() const = 0;
	virtual bool Submit(IKCommandBufferPtr commandBuffer, std::vector<IKSemaphorePtr> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence) = 0;
};