#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKBuffer.h"

#include <vector>

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
		vertexFormats.clear();
		vertexBuffers.clear();
		vertexStart = 0;
		vertexCount = 0;
	}
};
typedef std::shared_ptr<KVertexData> KVertexDataPtr;

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
		indexBuffer = nullptr;
		indexStart = 0;
		indexCount = 0;
	}
};
typedef std::shared_ptr<KIndexData> KIndexDataPtr;

struct KRenderCommand
{
	KVertexDataPtr vertexData;
	KIndexDataPtr indexData;
	IKPipelinePtr pipeline;
	IKPipelineHandlePtr pipelineHandle;
	bool indexDraw;
};

typedef std::vector<KRenderCommand> KRenderCommandList;