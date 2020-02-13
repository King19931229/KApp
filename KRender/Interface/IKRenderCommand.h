#pragma once

#include "Interface/IKShader.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKPipeline.h"

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

	~KVertexData()
	{
	}

	void Clear()
	{
		vertexBuffers.clear();
		vertexFormats.clear();
		vertexStart = 0;
		vertexCount = 0;
	}

	void Destroy()
	{
		assert(vertexFormats.size() == vertexBuffers.size());
		for (IKVertexBufferPtr& buffer : vertexBuffers)
		{
			buffer->UnInit();
			buffer = nullptr;
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

	~KIndexData()
	{
	}

	void Clear()
	{
		indexBuffer = nullptr;
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

	IKPipelinePtr pipeline;
	IKPipelineHandlePtr pipelineHandle;

	void* objectData;
	uint32_t objectPushOffset;
	bool useObjectData;

	bool indexDraw;

	KRenderCommand()
	{
		vertexData = nullptr;
		indexData = nullptr;

		pipeline = nullptr;
		pipelineHandle = nullptr;

		objectData = nullptr;
		objectPushOffset = 0;
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
		if(!pipelineHandle)
		{
			return false;
		}
		return true;
	}
};

typedef std::vector<KRenderCommand> KRenderCommandList;