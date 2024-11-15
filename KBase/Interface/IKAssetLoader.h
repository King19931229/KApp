#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/IKCodec.h"
#include "glm/glm.hpp"
#include <memory>
#include <vector>
#include <string>

enum AssetVertexComponent
{
	AVC_POSITION_3F,
	AVC_NORMAL_3F,

	AVC_UV_2F,
	AVC_UV2_2F,

	AVC_COLOR0_3F,
	AVC_COLOR1_3F,
	AVC_COLOR2_3F,
	AVC_COLOR3_3F,
	AVC_COLOR4_3F,
	AVC_COLOR5_3F,

	AVC_TANGENT_3F,
	AVC_BINORMAL_3F
};

enum
{
	MAX_COLOR_CHANNEL = AVC_COLOR5_3F - AVC_COLOR0_3F + 1
};
static_assert(AVC_COLOR0_3F + MAX_COLOR_CHANNEL == AVC_TANGENT_3F, "check");

typedef std::vector<AssetVertexComponent> KAssetVertexComponentGroup;

enum MeshTextureSemantic
{
	MTS_DIFFUSE = 0,
	MTS_BASE_COLOR = 0,

	MTS_NORMAL = 1,

	// Specular glosiness workflow
	MTS_SPECULAR_GLOSINESS = 2,
	// Metal roughness workflow
	MTS_METAL_ROUGHNESS = 2,

	MTS_EMISSIVE = 3,

	MTS_AMBIENT_OCCLUSION = 4,

	MTS_COUNT
};

enum MeshTextureFilter
{
	MTF_LINEAR,
	MTF_CLOSEST
};

enum MeshTextureAddress
{
	MTA_REPEAT,
	MTA_CLAMP_TO_EDGE,
	MTA_MIRRORED_REPEAT
};

enum MaterialAlphaMode
{
	MAM_OPAQUE,
	MAM_MASK,
	MAM_BLEND
};

struct KMeshTextureSampler
{
	MeshTextureFilter minFilter, magFilter;
	MeshTextureAddress addressModeU, addressModeV, addressModeW;

	KMeshTextureSampler()
	{
		minFilter = magFilter = MTF_LINEAR;
		addressModeU = addressModeV = addressModeW = MTA_REPEAT;
	}
};

struct KMeshRawData
{
	// For userdata
	std::vector<KAssetVertexComponentGroup> components;

	typedef std::vector<char> VertexDataBuffer;
	std::vector<VertexDataBuffer> verticesDatas;

	uint32_t vertexCount;

	std::vector<char> indicesData;
	uint32_t indexCount;
	bool index16Bit;

	struct Material
	{
		std::string urls[MTS_COUNT];
		KCodecResult codecs[MTS_COUNT];
		uint32_t virtualTileNums[MTS_COUNT] = { 0 };

		KMeshTextureSampler samplers[MTS_COUNT];

		MaterialAlphaMode alphaMode;

		float alphaMask;
		float alphaMaskCutoff;
		float metallicFactor;
		float roughnessFactor;
		glm::vec4 baseColorFactor;
		glm::vec4 emissiveFactor;

		bool metalWorkFlow;
		bool doubleSided;

		struct TexCoordSets
		{
			uint8_t baseColor = 0;
			uint8_t metallicRoughness = 0;
			uint8_t specularGlossiness = 0;
			uint8_t normal = 0;
			uint8_t occlusion = 0;
			uint8_t emissive = 0;
		} texCoordSets;

		struct Extension
		{
			glm::vec4 diffuseFactor;
			glm::vec4 specularFactor;
		} extension;

		Material()
		{
			alphaMode = MAM_OPAQUE;
			alphaMask = 0.0f;
			alphaMaskCutoff = 1.0f;
			metallicFactor = 1.0f;
			roughnessFactor = 1.0f;
			baseColorFactor = glm::vec4(1.0f);
			emissiveFactor = glm::vec4(1.0f);
			metalWorkFlow = false;
			doubleSided = false;
			extension.diffuseFactor = glm::vec4(1.0f);
			extension.specularFactor = glm::vec4(0.0f);
		}
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

	KMeshRawData()
	{
		vertexCount = 0;
		indexCount = 0;
		index16Bit =  false;
	}
};

struct KAssetImportOption
{
	std::vector<KAssetVertexComponentGroup> components;
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
	virtual bool Import(const char* pszFile, const KAssetImportOption& importOption, KMeshRawData& result) = 0;
};

namespace KAssetLoader
{
	extern bool CreateAssetLoaderManager();
	extern bool DestroyAssetLoaderManager();
	extern IKAssetLoaderPtr GetLoader(const char* pszFile);
}