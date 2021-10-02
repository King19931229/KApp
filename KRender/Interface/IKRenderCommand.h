#pragma once

#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKBuffer.h"
#include "KRender/Interface/IKPipeline.h"
#include "KBase/Publish/KAABBBox.h"

struct KVertexData
{
	// 每个顶点格式占用一个顶点缓冲
	std::vector<VertexFormat> vertexFormats;
	std::vector<IKVertexBufferPtr> vertexBuffers;

	// 顶点描述数据
	uint32_t vertexStart;
	uint32_t vertexCount;

	// 包围盒
	KAABBBox bound;

	KVertexData()
	{
		vertexStart = 0;
		vertexCount = 0;
		bound.SetNull();
	}

	~KVertexData()
	{
	}

	void Reset()
	{
		vertexBuffers.clear();
		vertexFormats.clear();
		bound.SetNull();
		vertexStart = 0;
		vertexCount = 0;
	}

	void Destroy()
	{
		SAFE_UNINIT_CONTAINER(vertexBuffers);
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

	void Reset()
	{
		indexBuffer = nullptr;
		indexStart = 0;
		indexCount = 0;
	}

	void Destroy()
	{
		SAFE_UNINIT(indexBuffer);
		indexStart = 0;
		indexCount = 0;
	}
};

struct KDynamicConstantBufferUsage
{
	IKUniformBufferPtr buffer;
	size_t offset;
	size_t range;
	size_t binding;

	KDynamicConstantBufferUsage()
	{
		buffer = nullptr;
		offset = 0;
		range = 0;
		binding = 0;
	}
};

struct KInstanceBufferUsage
{
	IKVertexBufferPtr buffer;
	size_t start;
	size_t count;
	size_t offset;

	KInstanceBufferUsage()
	{
		buffer = nullptr;
		start = 0;
		count = 0;
		offset = 0;
	}
};

struct KRenderCommand
{
	const KVertexData* vertexData;
	const KIndexData* indexData;

	IKPipelinePtr pipeline;
	IKPipelineHandlePtr pipelineHandle;

	bool indexDraw;
	bool instanceDraw;

	KDynamicConstantBufferUsage objectUsage;
	KDynamicConstantBufferUsage vertexShadingUsage;
	KDynamicConstantBufferUsage fragmentShadingUsage;
	std::vector<KInstanceBufferUsage> instanceUsages;

	KRenderCommand()
	{
		vertexData = nullptr;
		indexData = nullptr;

		pipeline = nullptr;
		pipelineHandle = nullptr;

		indexDraw = false;
		instanceDraw = false;
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
		if (instanceDraw && instanceUsages.empty())
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