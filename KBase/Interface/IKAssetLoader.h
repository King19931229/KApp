#pragma once
#include "KBase/Publish/KConfig.h"
#include <memory>
#include <vector>
#include <string>

enum AssetVertexComponent
{
	AVC_POSITION_3F = 0x0,
	AVC_NORMAL_3F = 0x1,
	AVC_UV_2F = 0x3,

	AVC_UV2_2F = 0x4,

	AVC_DIFFUSE_3F = 0x5,
	AVC_SPECULAR_3F = 0x6,

	AVC_TANGENT_3F = 0x7,
	AVC_BINORMAL_3F = 0x8,
};

struct KAssetImportResult
{
	typedef std::vector<char> VertexDataBuffer;

	std::vector<VertexDataBuffer> verticesDatas;
	uint32_t vertexCount;

	std::vector<char> indicesData;
	uint32_t indexCount;
	bool index16Bit;

	struct Material
	{
		std::string diffuse;
		std::string specular;
		std::string normal;
	};

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
		Material material;

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
	typedef std::vector<AssetVertexComponent> ComponentGroup;
	std::vector<ComponentGroup> components;
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