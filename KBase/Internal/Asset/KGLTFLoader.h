#pragma once
#include "Interface/IKAssetLoader.h"
#include "Interface/IKCodec.h"
#include "Publish/KAABBBox.h"
#include "stb_image_write.h"
#include "tiny_gltf.h"
#include "glm/gtc/quaternion.hpp"

class KGLTFLoader : public IKAssetLoader
{
protected:
	std::vector<KMeshTextureSampler> m_Samplers;

	struct Texture
	{
		std::string url;
		KCodecResult codec;
		KMeshTextureSampler sampler;
	};
	std::vector<Texture> m_Textures;

	struct Material
	{
		MaterialAlphaMode alphaMode = MAM_OPAQUE;
		float alphaMask = 0.0f;
		float alphaMaskCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		glm::vec4 emissiveFactor = glm::vec4(1.0f);
		Texture* baseColorTexture = nullptr;
		Texture* metallicRoughnessTexture = nullptr;
		Texture* normalTexture = nullptr;
		Texture* occlusionTexture = nullptr;
		Texture* emissiveTexture = nullptr;
		bool doubleSided = false;
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
			Texture* specularGlossinessTexture = nullptr;
			Texture* diffuseTexture = nullptr;
			glm::vec4 diffuseFactor = glm::vec4(1.0f);
			glm::vec3 specularFactor = glm::vec3(0.0f);
		} extension;
		struct PbrWorkflows
		{
			bool metallicRoughness = true;
			bool specularGlossiness = false;
		} pbrWorkflows;
	};
	std::vector<Material> m_Materials;

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 binormal;
		glm::vec2 uv0;
		glm::vec2 uv1;
		glm::vec4 joint0;
		glm::vec4 weight0;
		glm::vec4 color0;
		glm::vec4 color1;

		bool operator<(const Vertex& rhs) const
		{
			const float eps = 1e-6f;
			float dot = 0;
#define COMPARE_AND_RETURN(member)\
			dot = glm::dot(member - rhs.member, member - rhs.member);\
			if (dot > eps)\
				return memcmp(&member, &rhs.member, sizeof(member)) < 0;

			COMPARE_AND_RETURN(pos);
			COMPARE_AND_RETURN(normal);
			COMPARE_AND_RETURN(tangent);
			COMPARE_AND_RETURN(binormal);
			COMPARE_AND_RETURN(uv0);
			COMPARE_AND_RETURN(uv1);
			COMPARE_AND_RETURN(joint0);
			COMPARE_AND_RETURN(weight0);
			COMPARE_AND_RETURN(color0);
			COMPARE_AND_RETURN(color1);

#undef COMPARE_AND_RETURN
			return false;
		}
	};

	struct Primitive
	{
		uint32_t firstIndex{ 0 };
		uint32_t indexCount{ 0 };
		uint32_t firstvertex{ 0 };
		uint32_t vertexCount{ 0 };
		Material& material;
		bool hasIndices{ false };
		KAABBBox bb;
		Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t firstVertex, uint32_t vertexCount, Material& material);
		void SetBoundingBox(const glm::vec3& min, const glm::vec3& max);
	};
	typedef std::shared_ptr<Primitive> PrimitivePtr;

	static constexpr size_t MAX_NUM_JOINTS = 128;

	struct Mesh
	{
		KAABBBox bb;
		KAABBBox aabb;
		std::vector<PrimitivePtr> primitives;
		glm::mat4 matrix;
		glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
		float jointcount{ 0 };
		Mesh(const glm::mat4& matrix);
		void SetBoundingBox(glm::vec3 min, glm::vec3 max);
	};
	typedef std::shared_ptr<Mesh> MeshPtr;

	struct MeshLoad
	{
		std::vector<uint32_t> indexBuffer;
		std::vector<Vertex> vertexBuffer;
		size_t indexPos = 0;
		size_t vertexPos = 0;
	} loaderInfo;

	struct Node;
	typedef std::shared_ptr<Node> NodePtr;

	struct Node
	{
		Node* parent = nullptr;
		uint32_t index = 0;
		std::vector<NodePtr> children;
		glm::mat4 matrix;
		std::string name;
		MeshPtr mesh;
		int32_t skinIndex = -1;
		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f };
		glm::quat rotation{};
		KAABBBox bvh;
		KAABBBox aabb;
		glm::mat4 LocalMatrix();
		glm::mat4 GetMatrix();
		void Update();
	};

	// Root nodes
	std::vector<NodePtr> m_Nodes;
	// All nodes
	std::vector<NodePtr> m_LinearNodes;

	struct Dimensions
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
	} m_Dimensions;

	void LoadTextureSamplers(tinygltf::Model& gltfModel);
	void LoadTextures(tinygltf::Model& gltfModel);
	void LoadMaterials(tinygltf::Model& gltfModel);
	void LoadAnimations(tinygltf::Model& gltfModel);
	void LoadSkins(tinygltf::Model& gltfModel);

	void CalculateBoundingBox(NodePtr node, Node* parent);
	void SetSceneDimensions();
	void TransformMeshVertices();

	MeshTextureFilter GetTextureFilter(int32_t filterMode);
	MeshTextureAddress GetTextureAddress(int32_t addressMode);
	KCodecResult GetCodecResult(tinygltf::Image& gltfimage);
	bool IsBinary(const char* pszFile) const;

	void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
	void LoadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, MeshLoad& loaderInfo, float globalscale);

	bool LoadModel(const char* pszFile, tinygltf::Model& gltfModel);

	bool AppendMeshIntoResult(NodePtr node, KAssetImportResult& result);
	bool ConvertIntoResult(const KAssetImportOption& importOption, KAssetImportResult& result);
public:
	KGLTFLoader();
	~KGLTFLoader();

	virtual bool Import(const char* pszFile, const KAssetImportOption& importOption, KAssetImportResult& result);

	static bool Init();
	static bool UnInit();
};