#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKShader.h"
#include "Interface/IKBuffer.h"

#include <assert.h>

struct KVertexData
{
	// 每个顶点格式占用一个顶点缓冲
	std::vector<VertexFormat> vertexFormats;
	std::vector<IKVertexBufferPtr> vertexBuffers;
	uint32_t vertexStart;
	uint32_t vertexCount;

	KVertexData()
	{
		vertexStart = 0;
		vertexCount = 0;
	}

	void Destroy()
	{
		assert(vertexFormats.size() == vertexBuffers.size());
		for(IKVertexBufferPtr& buffer : vertexBuffers)
		{
			buffer->UnInit();
		}
		vertexBuffers.clear();
		vertexFormats.clear();		
		vertexStart = 0;
		vertexCount = 0;
	}
};

struct KIndexData
{
	IKIndexBufferPtr indexBuffer;
	uint32_t indexStart;
	uint32_t indexCount;

	KIndexData()
	{
		indexStart = 0;
		indexCount = 0;
	}

	void Destroy()
	{
		if(indexBuffer)
		{
			indexBuffer->UnInit();
			indexBuffer = nullptr;
		}
		indexStart = 0;
		indexCount = 0;
	}
};

struct KRenderCommand
{
	const KVertexData* vertexData;
	const KIndexData* indexData;
	const IKPipeline* pipeline;
	const void* objectData;
	bool useObjectData;
	bool indexDraw;

	KRenderCommand()
	{
		vertexData = nullptr;
		indexData = nullptr;
		pipeline = nullptr;
		objectData = false;
		useObjectData = false;
		indexDraw = false;
	}

	bool Complete() const
	{
		if(!vertexData || !pipeline)
		{
			return false;
		}
		if(indexDraw && !indexData)
		{
			return false;
		}
		if(useObjectData && !objectData)
		{
			return false;
		}
		return true;
	}
};

typedef std::vector<KRenderCommand> KRenderCommandList;

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindowPtr window) = 0;
	virtual bool UnInit() = 0;

	virtual bool CreateShader(IKShaderPtr& shader) = 0;
	virtual bool CreateProgram(IKProgramPtr& program) = 0;

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer) = 0;
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer) = 0;
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer) = 0;

	virtual bool CreateTexture(IKTexturePtr& texture) = 0;
	virtual bool CreateSampler(IKSamplerPtr& sampler) = 0;

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target) = 0;
	virtual bool CreatePipeline(IKPipelinePtr& pipeline) = 0;
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle) = 0;

	virtual bool CreateUIOVerlay(IKUIOverlayPtr& ui) = 0;

	// TODO abstract CommandBuffer
	virtual bool Render(void* commandBufferPtr, IKRenderTarget* target, size_t frameIndex, size_t threadIndex, const KRenderCommand& command) = 0;

	virtual bool Present() = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform); 