#pragma once

#include "Interface/IKShader.h"
#include "Interface/IKBuffer.h"
#include <assert.h>

struct KVertexData
{
	// ÿ�������ʽռ��һ�����㻺��
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

	IKPipeline* pipeline;
	IKPipelineHandle* pipelineHandle;

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