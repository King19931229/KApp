#pragma once
#include "KRender/Interface/IKRenderConfig.h"

struct IKSemaphore
{
	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};

struct IKQueue
{
	virtual bool Init(QueueCategory category, uint32_t queueIndex) = 0;
	virtual bool UnInit() = 0;
	virtual QueueCategory GetCategory() const = 0;
	virtual uint32_t GetIndex() const = 0;
	virtual bool Submit(IKCommandBufferPtr commandBuffer, IKSemaphorePtr wait, IKSemaphorePtr singal) = 0;
};