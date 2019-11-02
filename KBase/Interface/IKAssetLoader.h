#pragma once
#include "Publish/KConfig.h"
#include <memory>
#include <vector>

enum AssetVertexComponent
{
	AVC_POSITION_3F = 0x0,
	AVC_NORMAL_3F = 0x1,
	AVC_COLOR_3F = 0x2,
	AVC_UV_2F = 0x3,
	AVC_TANGENT_3F = 0x4,
	AVC_BITANGENT_3F = 0x5,
};

struct KAssetImportResult
{
	std::vector<char> verticesData;
	uint32_t vertexCount;

	std::vector<char> indicesData;
	uint32_t indexCount;
	bool index16Bit;

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;

		ModelPart()
		{
			vertexBase = 0;
			vertexCount = 0;
			indexBase = 0;
			indexCount = 0;
		}
	};
	std::vector<ModelPart> parts;

	struct Extend
	{
		float min[3];
		float max[3];
		Extend()
		{
			min[0] = min[1] = min[2] = std::numeric_limits<float>::max();
			max[0] = max[1] = max[2] = std::numeric_limits<float>::min();
		}
	} extend;

	KAssetImportResult()
	{
		vertexCount = 0;
		indexCount = 0;
		index16Bit =  false;
	}
};

struct KAssetImportOption
{
	std::vector<AssetVertexComponent> components;
	float scale[3];
	float center[3];
	float uvScale[2];
	KAssetImportOption()
	{
		scale[0] = scale[1] = scale[2] = 1.0f;
		center[0] = center[1] = center[2] = 0.0f;
		uvScale[0] = uvScale[1] = 1.0f;
	}
};

struct IKAssetLoader;
typedef std::shared_ptr<IKAssetLoader> IKAssetLoaderPtr;

struct IKAssetLoader
{
	virtual bool Import(const char* pszFile, const KAssetImportOption& importOption, KAssetImportResult& result) = 0;
};

EXPORT_DLL bool InitAssetLoaderManager();
EXPORT_DLL bool UnInitAssetLoaderManager();
EXPORT_DLL IKAssetLoaderPtr GetAssetLoader();