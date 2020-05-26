#pragma once

#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKBuffer.h"
#include "KRender/Interface/IKPipeline.h"
#include "KBase/Publish/KAABBBox.h"

#include <assert.h>

struct KVertexData
{
	// 每个顶点格式占用一个顶点缓冲
	std::vector<VertexFormat> vertexFormats;
	std::vector<IKVertexBufferPtr> vertexBuffers;
	IKVertexBufferPtr instanceBuffer;
	uint32_t vertexStart;
	uint32_t vertexCount;
	KAABBBox bound;

	KVertexData()
	{
		vertexStart = 0;
		vertexCount = 0;
		instanceBuffer = nullptr;
		bound.SetNull();
	}

	~KVertexData()
	{
	}

	void Clear()
	{
		vertexBuffers.clear();
		vertexFormats.clear();
		bound.SetNull();
		vertexStart = 0;
		vertexCount = 0;
		instanceBuffer = nullptr;
	}

	void Destroy()
	{
		assert(vertexFormats.size() == vertexBuffers.size());
		for (IKVertexBufferPtr& buffer : vertexBuffers)
		{
			buffer->UnInit();
			buffer = nullptr;
		}
		if (instanceBuffer)
		{
			instanceBuffer->UnInit();
			instanceBuffer = nullptr;
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

// TODO 分离KRenderData 与 KRenderCommand
struct KRenderCommand
{
	const KVertexData* vertexData;
	const KIndexData* indexData;

	IKPipelinePtr pipeline;
	IKPipelineHandlePtr pipelineHandle;

	std::vector<char> objectData;
	bool indexDraw;

	KRenderCommand()
	{
		vertexData = nullptr;
		indexData = nullptr;

		pipeline = nullptr;
		pipelineHandle = nullptr;

		indexDraw = false;
	}

	template<typename T>
	void SetObjectData(T&& value)
	{
		objectData.resize(sizeof(value));
		memcpy(objectData.data(), &value, objectData.size());
	}

	void ClearObjectData()
	{
		objectData.clear();
		objectData.shrink_to_fit();
	}

	bool Complete() const
	{
		if (!vertexData || !pipeline)
		{
			return false;
		}
		if (indexDraw && !indexData)
		{
			return false;
		}
		if (!pipelineHandle)
		{
			return false;
		}
		return true;
	}
};

typedef std::vector<KRenderCommand> KRenderCommandList;