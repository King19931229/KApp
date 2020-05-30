#pragma once

#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKBuffer.h"
#include "KRender/Interface/IKPipeline.h"
#include "KBase/Publish/KAABBBox.h"

#include <assert.h>

enum InstanceBufferStage
{
	IBS_PRE_Z,
	IBS_OPAQUE,

	IBS_CSM0,
	IBS_CSM1,
	IBS_CSM2,
	IBS_CSM3,

	IBS_COUNT,
	IBS_UNKNOWN = IBS_COUNT
};

static constexpr const size_t MAX_FRAME_IN_FLIGHT_INSTANCE = 4;

struct KVertexData
{
	// 每个顶点格式占用一个顶点缓冲
	std::vector<VertexFormat> vertexFormats;
	std::vector<IKVertexBufferPtr> vertexBuffers;

	// TODO 建立InstanceBuffer Manager管理
	IKVertexBufferPtr instanceBuffers[MAX_FRAME_IN_FLIGHT_INSTANCE][IBS_COUNT];
	size_t instanceDataHash[MAX_FRAME_IN_FLIGHT_INSTANCE][IBS_COUNT];
	size_t instanceCount[MAX_FRAME_IN_FLIGHT_INSTANCE][IBS_COUNT];

	// 顶点描述数据
	uint32_t vertexStart;
	uint32_t vertexCount;

	// 包围盒
	KAABBBox bound;

	KVertexData()
	{
		vertexStart = 0;
		vertexCount = 0;
		for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT_INSTANCE; ++i)
		{
			ZERO_ARRAY_MEMORY(instanceBuffers[i]);
			ZERO_ARRAY_MEMORY(instanceDataHash[i]);
			ZERO_ARRAY_MEMORY(instanceCount[i]);
		}
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
		for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT_INSTANCE; ++i)
		{
			ZERO_ARRAY_MEMORY(instanceBuffers[i]);
			ZERO_ARRAY_MEMORY(instanceDataHash[i]);
			ZERO_ARRAY_MEMORY(instanceCount[i]);
		}
	}

	void Destroy()
	{
		assert(vertexFormats.size() == vertexBuffers.size());
		SAFE_UNINIT_CONTAINER(vertexBuffers);
		for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT_INSTANCE; ++i)
		{
			SAFE_UNINIT_CONTAINER(instanceBuffers[i]);
			ZERO_ARRAY_MEMORY(instanceDataHash[i]);
			ZERO_ARRAY_MEMORY(instanceCount[i]);
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
		SAFE_UNINIT(indexBuffer);
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

	uint32_t instanceCount;
	IKVertexBufferPtr instanceBuffer;

	std::vector<char> objectData;

	bool indexDraw;
	bool instanceDraw;

	KRenderCommand()
	{
		vertexData = nullptr;
		indexData = nullptr;

		pipeline = nullptr;
		pipelineHandle = nullptr;

		instanceCount = 0;
		instanceBuffer = nullptr;

		indexDraw = false;
		instanceDraw = false;
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
		if (instanceDraw && (!instanceBuffer || !instanceCount))
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