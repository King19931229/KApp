#pragma once
#include "Interface/IKRenderDevice.h"
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "KBase/Interface/IKRunable.h"

struct KRHICommandBase;
class KRHICommandList;

typedef std::shared_ptr<KRHICommandBase> KRHICommandBasePtr;

struct KRHICommandBase
{
	KRHICommandBasePtr next;
	virtual void Execute(KRHICommandList& commandList) = 0;
	virtual const char* GetName() = 0;
};

typedef std::shared_ptr<KRHICommandBase> KRHICommandBasePtr;

#define RHICOMMAND_DEFINE(CommandName)\
struct KRHI##CommandName##Command : public KRHICommandBase\
{\
	virtual const char* GetName() { return #CommandName; }\
};\
struct CommandName : public KRHI##CommandName##Command\

RHICOMMAND_DEFINE(KUpdateUniformBuffer)
{
	IKUniformBufferPtr uniformBuffer;
	std::vector<char> data;
	uint32_t offset;
	uint32_t size;

	KUpdateUniformBuffer(IKUniformBufferPtr inUniformBuffer, void* inData, uint32_t inOffset, uint32_t inSize)
		: uniformBuffer(inUniformBuffer)
		, offset(inOffset)
		, size(inSize)
	{
		data.resize(size);
		memcpy(data.data(), inData, inSize);
	}

	virtual void Execute(KRHICommandList& commandList) override;
};

namespace RHICommandFlush
{
	enum Type
	{
		DispatchToRHIThread,
		FlushRHIThread
	};
}

struct KRHICommandTaskWork : public IKTaskWork
{
	KRHICommandList& commandList;
	KRHICommandBasePtr commandHead;

	KRHICommandTaskWork(KRHICommandList& inCommandList, KRHICommandBasePtr inCommandHead)
		: commandList(inCommandList)
		, commandHead(inCommandHead)
	{}

	virtual void DoWork() override
	{
		while (commandHead)
		{
			commandHead->Execute(commandList);
			commandHead = commandHead->next;
		}
	}

	virtual const char* GetDebugInfo() { return "KRHICommandTaskWork"; }
};

class KRHIThread : public IKRunable
{
protected:
	bool m_Stop;
public:
	KRHIThread();
	~KRHIThread();

	virtual void StartUp() override;
	virtual void Run() override;
	virtual void ShutDown() override;
};

class KRHICommandList
{
protected:
	IKCommandBufferPtr m_CommandBuffer;
	KRHICommandBasePtr m_CommandHead;
	KRHICommandBasePtr* m_CommandNext;
	IKGraphTaskEventRef m_AsyncTask;
	bool m_ImmediateMode;
public:
	KRHICommandList();
	~KRHICommandList();

	inline void SetCommandBuffer(IKCommandBufferPtr commandBuffer) { m_CommandBuffer = commandBuffer; }
	inline IKCommandBufferPtr GetCommandBuffer() { return m_CommandBuffer; }

	void SetImmediate(bool immediate);

	void Flush(RHICommandFlush::Type flushType);

	void UpdateUniformBuffer(IKUniformBufferPtr uniformBuffer, void* data, uint32_t offset, uint32_t size);
};