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
		Reset();
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
		Reset();
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

struct KMeshData
{
	uint32_t groupCountX;
	uint32_t groupCountY;
	uint32_t groupCountZ;

	KMeshData()
	{
		Reset();
	}

	~KMeshData()
	{
	}

	void Reset()
	{
		groupCountX = 0;
		groupCountY = 0;
		groupCountZ = 0;
	}
};

struct KMeshDataLeagcy
{
	IKVertexBufferPtr meshletDescBuffer;
	IKVertexBufferPtr meshletPrimBuffer;
	uint32_t offset;
	uint32_t count;

	KMeshDataLeagcy()
	{
		Reset();
	}

	~KMeshDataLeagcy()
	{
	}

	void Reset()
	{
		meshletDescBuffer = nullptr;
		meshletPrimBuffer = nullptr;
		offset = 0;
		count = 0;
	}

	void Destroy()
	{
		SAFE_UNINIT(meshletDescBuffer);
		SAFE_UNINIT(meshletPrimBuffer);
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

struct KStorageBufferUsage
{
	IKVertexBufferPtr buffer;
	size_t binding;

	KStorageBufferUsage()
	{
		buffer = nullptr;
		binding = 0;
	}
};

struct KRenderCommand
{
	const KVertexData* vertexData;
	const KIndexData* indexData;
	const KMeshData* meshData;
	const struct IKMaterialTextureBinding* textureBinding;

	uint32_t threadIndex;

	uint32_t indirectOffset;
	uint32_t indirectCount;
	IKStorageBufferPtr indirectArgsBuffer;

	IKPipelinePtr pipeline;
	IKPipelineHandlePtr pipelineHandle;

	bool indexDraw;
	bool instanceDraw;
	bool meshShaderDraw;
	bool indirectDraw;

	std::vector<KDynamicConstantBufferUsage> dynamicConstantUsages;
	std::vector<KInstanceBufferUsage> instanceUsages;
	std::vector<KStorageBufferUsage> meshStorageUsages;

	KRenderCommand()
	{
		vertexData = nullptr;
		indexData = nullptr;
		meshData = nullptr;
		textureBinding = nullptr;

		threadIndex = 0;
		indirectOffset = 0;
		indirectCount = 0;
		indirectArgsBuffer = nullptr;

		pipeline = nullptr;
		pipelineHandle = nullptr;

		indexDraw = false;
		instanceDraw = false;
		meshShaderDraw = false;
		indirectDraw = false;
	}

	bool Complete() const
	{
		if (!(vertexData || indirectDraw) || !pipeline)
		{
			return false;
		}
		if (indirectDraw && !indirectArgsBuffer)
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
		if (meshShaderDraw && !(meshData || indirectDraw))
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