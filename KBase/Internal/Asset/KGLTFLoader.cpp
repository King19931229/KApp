#include "KGLTFLoader.h"
#include "Interface/IKLog.h"
#include "Interface/IKFileSystem.h"
#include "Publish/KFileTool.h"
#include "glm/gtc/type_ptr.inl"
#include <algorithm>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

/*
* Core code come from:
* https://github.com/SaschaWillems/Vulkan-glTF-PBR
* Copyright (c) 2018 Sascha Willems
*/

bool KGLTFLoader::Init()
{
	return true;
}

bool KGLTFLoader::UnInit()
{
	return true;
}

KGLTFLoader::KGLTFLoader()
{
}

KGLTFLoader::~KGLTFLoader()
{
}


KGLTFLoader::Primitive::Primitive(uint32_t InFirstIndex, uint32_t InIndexCount, uint32_t inFirstVertex, uint32_t InVertexCount, Material& InMaterial)
	: firstIndex(InFirstIndex)
	, indexCount(InIndexCount)
	, firstvertex(inFirstVertex)
	, vertexCount(InVertexCount)
	, material(InMaterial)
{
}

void KGLTFLoader::Primitive::SetBoundingBox(const glm::vec3& min, const glm::vec3& max)
{
	bb.InitFromMinMax(min, max);
}

KGLTFLoader::Mesh::Mesh(const glm::mat4& inMatrix)
	: matrix(inMatrix)
{
}

void KGLTFLoader::Mesh::SetBoundingBox(glm::vec3 min, glm::vec3 max)
{
	bb.InitFromMinMax(min, max);
}

MeshTextureFilter KGLTFLoader::GetTextureFilter(int32_t filterMode)
{
	/* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_sampler_minfilter
	* 9728 NEAREST
	* 9729 LINEAR
	* 9984 NEAREST_MIPMAP_NEAREST
	* 9985 LINEAR_MIPMAP_NEAREST
	* 9986 NEAREST_MIPMAP_LINEAR
	* 9987 LINEAR_MIPMAP_LINEAR
	*/

	switch (filterMode)
	{
		case 9728:
			return MTF_CLOSEST;
		case 9729:
			return MTF_LINEAR;
		case 9984:
			return MTF_CLOSEST;
		case 9985:
			return MTF_CLOSEST;
		case 9986:
			return MTF_LINEAR;
		case 9987:
		case -1:
			return MTF_LINEAR;
	}

	KLog::Logger->Log(LL_ERROR, "Unknown filter mode %d", filterMode);
	return MTF_CLOSEST;
}

MeshTextureAddress KGLTFLoader::GetTextureAddress(int32_t addressMode)
{
	/* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_sampler_wraps
	* 33071 CLAMP_TO_EDGE
	* 33648 MIRRORED_REPEAT
	* 10497 REPEAT
	*/

	switch (addressMode)
	{
		case 33071:
			return MTA_CLAMP_TO_EDGE;
		case 33648:
			return MTA_MIRRORED_REPEAT;
		case -1:
		case 10497:
			return MTA_REPEAT;
	}

	KLog::Logger->Log(LL_ERROR, "Unknown address mode %d", addressMode);
	return MTA_REPEAT;
}

KCodecResult KGLTFLoader::GetCodecResult(tinygltf::Image& gltfimage)
{
	std::vector<unsigned char> bufferData;

	size_t bufferSize = 0;
	if (gltfimage.component == 3)
	{
		// Most devices don't support RGB only on Vulkan so convert if necessary
		bufferSize = gltfimage.width * gltfimage.height * 4;
		bufferData.resize(bufferSize);
		unsigned char* rgba = bufferData.data();
		unsigned char* rgb = &gltfimage.image[0];
		for (int32_t i = 0; i < gltfimage.width * gltfimage.height; ++i)
		{
			for (int32_t j = 0; j < 3; ++j)
			{
				rgba[j] = rgb[j];
			}
			rgba += 4;
			rgb += 3;
		}
	}
	else
	{
		bufferSize = gltfimage.image.size();
		bufferData.resize(bufferSize);
		memcpy(bufferData.data(), gltfimage.image.data(), bufferSize);
	}

	KCodecResult result;

	result.eFormat = IF_R8G8B8A8;
	result.uWidth = gltfimage.width;
	result.uHeight = gltfimage.height;
	result.uDepth = 1;
	result.uMipmap = 1;

	result.b3DTexture = false;
	result.bCompressed = false;
	result.bCubemap = false;

	result.pData = KImageDataPtr(new KImageData(bufferData.size()));
	memcpy(result.pData->GetData(), bufferData.data(), bufferData.size());

	KSubImageInfo subImageInfo;
	subImageInfo.uFaceIndex = 0;
	subImageInfo.uMipmapIndex = 0;;
	subImageInfo.uOffset = 0;
	subImageInfo.uSize = bufferData.size();
	subImageInfo.uWidth = gltfimage.width;
	subImageInfo.uHeight = gltfimage.height;

	result.pData->GetSubImageInfo().push_back(subImageInfo);

	return result;
}

bool KGLTFLoader::IsBinary(const char* pszFile) const
{
	std::string name, ext;
	if (KFileTool::SplitExt(pszFile, name, ext))
	{
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
		if (ext == "glb")
		{
			return true;
		}
	}
	return false;
}

void KGLTFLoader::LoadTextureSamplers(tinygltf::Model& gltfModel)
{
	for (tinygltf::Sampler smpl : gltfModel.samplers)
	{
		KMeshTextureSampler sampler{};
		sampler.minFilter = GetTextureFilter(smpl.minFilter);
		sampler.magFilter = GetTextureFilter(smpl.magFilter);
		sampler.addressModeU = GetTextureAddress(smpl.wrapS);
		sampler.addressModeV = GetTextureAddress(smpl.wrapT);
		sampler.addressModeW = sampler.addressModeV;
		m_Samplers.push_back(sampler);
	}
}

void KGLTFLoader::LoadTextures(tinygltf::Model& gltfModel)
{
	for (tinygltf::Texture& tex : gltfModel.textures)
	{
		tinygltf::Image image = gltfModel.images[tex.source];
		KMeshTextureSampler sampler;
		if (tex.sampler >= 0)
		{
			sampler = m_Samplers[tex.sampler];
		}
		else
		{
			sampler.minFilter = MTF_LINEAR;
			sampler.magFilter = MTF_LINEAR;
			sampler.addressModeU = MTA_REPEAT;
			sampler.addressModeV = MTA_REPEAT;
			sampler.addressModeW = MTA_REPEAT;
		}

		Texture texture;
		texture.codec = GetCodecResult(image);
		texture.sampler = sampler;
		m_Textures.push_back(texture);
	}
}

void KGLTFLoader::LoadMaterials(tinygltf::Model& gltfModel)
{
	for (tinygltf::Material& mat : gltfModel.materials)
	{
		Material material;

		material.doubleSided = mat.doubleSided;

		if (mat.values.find("baseColorTexture") != mat.values.end())
		{
			material.baseColorTexture = &m_Textures[mat.values["baseColorTexture"].TextureIndex()];
			material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
		}
		if (mat.values.find("metallicRoughnessTexture") != mat.values.end())
		{
			material.metallicRoughnessTexture = &m_Textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
			material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
		}
		if (mat.values.find("roughnessFactor") != mat.values.end())
		{
			material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
		}
		if (mat.values.find("metallicFactor") != mat.values.end())
		{
			material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
		}
		if (mat.values.find("baseColorFactor") != mat.values.end())
		{
			material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
		}
		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end())
		{
			material.normalTexture = &m_Textures[mat.additionalValues["normalTexture"].TextureIndex()];
			material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
		}
		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end())
		{
			material.emissiveTexture = &m_Textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
			material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
		}
		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end())
		{
			material.occlusionTexture = &m_Textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
			material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
		}
		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end())
		{
			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND")
			{
				material.alphaMode = MAM_BLEND;
			}
			if (param.string_value == "MASK")
			{
				material.alphaCutoff = 0.5f;
				material.alphaMode = MAM_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end())
		{
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}
		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end())
		{
			material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
		}

		// Extensions
		// @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers
		if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end())
		{
			auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
			if (ext->second.Has("specularGlossinessTexture"))
			{
				auto index = ext->second.Get("specularGlossinessTexture").Get("index");
				material.extension.specularGlossinessTexture = &m_Textures[index.Get<int>()];
				auto texCoordSet = ext->second.Get("m_Textures").Get("texCoord");
				material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
				material.pbrWorkflows.specularGlossiness = true;
			}
			if (ext->second.Has("diffuseTexture"))
			{
				auto index = ext->second.Get("diffuseTexture").Get("index");
				material.extension.diffuseTexture = &m_Textures[index.Get<int>()];
			}
			if (ext->second.Has("diffuseFactor"))
			{
				auto factor = ext->second.Get("diffuseFactor");
				for (uint32_t i = 0; i < factor.ArrayLen(); i++)
				{
					auto val = factor.Get(i);
					material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
				}
			}
			if (ext->second.Has("specularFactor"))
			{
				auto factor = ext->second.Get("specularFactor");
				for (uint32_t i = 0; i < factor.ArrayLen(); i++)
				{
					auto val = factor.Get(i);
					material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
				}
			}
		}

		m_Materials.push_back(material);
	}

	// Push a default material at the end of the list for meshes with no material assigned
	m_Materials.push_back(Material());
}

void KGLTFLoader::LoadAnimations(tinygltf::Model& gltfModel)
{
}

void KGLTFLoader::LoadSkins(tinygltf::Model& gltfModel)
{
}

bool KGLTFLoader::LoadModel(const char* pszFile, tinygltf::Model& gltfModel)
{
	tinygltf::TinyGLTF gltfContext;

	bool binary = IsBinary(pszFile);

	std::string error;
	std::string warning;

	KFileInformation fileInfo;
	IKDataStreamPtr dataStream = nullptr;
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	if (!(system && system->Open(pszFile, IT_FILEHANDLE, dataStream, &fileInfo)))
	{
		system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
		if (!(system && system->Open(pszFile, IT_FILEHANDLE, dataStream, &fileInfo)))
		{
			return false;
		}
	}

	std::vector<unsigned char> fileData;
	fileData.resize(dataStream->GetSize());
	dataStream->Read(fileData.data(), fileData.size());

	bool fileLoaded = binary ? gltfContext.LoadBinaryFromMemory(&gltfModel, &error, &warning, fileData.data(), (unsigned int)fileData.size(), fileInfo.parentFolder)
		: gltfContext.LoadASCIIFromString(&gltfModel, &error, &warning, (char*)fileData.data(), (unsigned int)fileData.size(), fileInfo.parentFolder);

	if (!warning.empty())
	{
		KLog::Logger->Log(LL_WARNING, "%s", warning.c_str());
	}

	if (!error.empty())
	{
		KLog::Logger->Log(LL_ERROR, "%s", error.c_str());
	}

	return fileLoaded;
}

void KGLTFLoader::GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
{
	if (node.children.size() > 0)
	{
		for (size_t i = 0; i < node.children.size(); i++)
		{
			GetNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
		}
	}
	if (node.mesh > -1)
	{
		const tinygltf::Mesh mesh = model.meshes[node.mesh];
		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			auto primitive = mesh.primitives[i];
			vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
			if (primitive.indices > -1)
			{
				indexCount += model.accessors[primitive.indices].count;
			}
		}
	}
}

void KGLTFLoader::LoadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, MeshLoad& loaderInfo, float globalscale)
{
	NodePtr newNode = NodePtr(new Node{});
	newNode->index = nodeIndex;
	newNode->parent = parent;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	// Generate local node matrix
	glm::vec3 translation = glm::vec3(0.0f);
	if (node.translation.size() == 3)
	{
		translation = glm::make_vec3(node.translation.data());
		newNode->translation = translation;
	}
	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4)
	{
		glm::quat q = glm::make_quat(node.rotation.data());
		newNode->rotation = glm::mat4(q);
	}
	glm::vec3 scale = glm::vec3(1.0f);
	if (node.scale.size() == 3)
	{
		scale = glm::make_vec3(node.scale.data());
		newNode->scale = scale;
	}
	if (node.matrix.size() == 16)
	{
		newNode->matrix = glm::make_mat4x4(node.matrix.data());
	};

	// Node with children
	if (node.children.size() > 0)
	{
		for (size_t i = 0; i < node.children.size(); i++)
		{
			LoadNode(newNode.get(), model.nodes[node.children[i]], node.children[i], model, loaderInfo, globalscale);
		}
	}

	// Node contains mesh data
	if (node.mesh > -1)
	{
		const tinygltf::Mesh mesh = model.meshes[node.mesh];
		MeshPtr newMesh = MeshPtr(new Mesh(newNode->matrix));
		for (size_t j = 0; j < mesh.primitives.size(); j++)
		{
			const tinygltf::Primitive& primitive = mesh.primitives[j];
			uint32_t vertexStart = static_cast<uint32_t>(loaderInfo.vertexPos);
			uint32_t indexStart = static_cast<uint32_t>(loaderInfo.indexPos);
			uint32_t indexCount = 0;
			uint32_t vertexCount = 0;
			glm::vec3 posMin{};
			glm::vec3 posMax{};
			bool hasSkin = false;
			bool hasTangent = false;
			bool hasIndices = primitive.indices > -1;
			// Vertices
			{
				const float* bufferPos = nullptr;
				const float* bufferNormals = nullptr;
				const float* bufferTangents = nullptr;
				const float* bufferTexCoordSet0 = nullptr;
				const float* bufferTexCoordSet1 = nullptr;
				const float* bufferColorSet0 = nullptr;
				const float* bufferColorSet1 = nullptr;
				const void* bufferJoints = nullptr;
				const float* bufferWeights = nullptr;

				int posByteStride = 0;
				int normByteStride = 0;
				int tangentByteStride = 0;
				int uv0ByteStride = 0;
				int uv1ByteStride = 0;
				int color0ByteStride = 0;
				int color1ByteStride = 0;
				int jointByteStride = 0;
				int weightByteStride = 0;

				int jointComponentType = 0;

				// Position attribute is required
				assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

				const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
				bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
				vertexCount = static_cast<uint32_t>(posAccessor.count);
				posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
				{
					const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
					bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
					normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
				{
					const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& tangentView = model.bufferViews[tangentAccessor.bufferView];
					bufferTangents = reinterpret_cast<const float*>(&(model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
					tangentByteStride = tangentAccessor.ByteStride(tangentView) ? (tangentAccessor.ByteStride(tangentView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				// UVs
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}
				if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
				{
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}

				// Vertex colors
				if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					bufferColorSet0 = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					color0ByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}
				if (primitive.attributes.find("COLOR_1") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_1")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					bufferColorSet1 = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					color1ByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				// Skinning
				// Joints
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
					bufferJoints = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
					jointComponentType = jointAccessor.componentType;
					jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
					bufferWeights = reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
					weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				hasSkin = (bufferJoints && bufferWeights);

				for (size_t v = 0; v < posAccessor.count; v++)
				{
					Vertex& vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
					vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
					vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
					vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
					vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
					vert.color0 = bufferColorSet0 ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);
					vert.color1 = bufferColorSet1 ? glm::make_vec4(&bufferColorSet1[v * color1ByteStride]) : glm::vec4(1.0f);

					glm::vec4 tangent = bufferTangents ? glm::make_vec4(&bufferTangents[v * tangentByteStride]) : glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

					// They are not perpendicular to each other, seek a truly perpendicular
					if (abs(glm::dot(vert.normal, glm::vec3(tangent.x, tangent.y, tangent.z))) > 0.1f)
					{
						tangent = glm::vec4(glm::normalize(glm::cross(vert.normal, glm::vec3(tangent.x, tangent.y, tangent.z))), 1.0f);
					}

					vert.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
					// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
					vert.binormal = glm::normalize(glm::cross(vert.normal, vert.tangent)) * tangent.w;

					if (hasSkin)
					{
						switch (jointComponentType)
						{
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							{
								const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
								vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
								break;
							}
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							{
								const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
								vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
								break;
							}
							default:
							{
								// Not supported by spec
								KLog::Logger->Log(LL_ERROR, "Joint component type %d not supported!", jointComponentType);
								break;
							}
						}
					}
					else
					{
						vert.joint0 = glm::vec4(0.0f);
					}
					vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
					// Fix for all zero weights
					if (glm::length(vert.weight0) == 0.0f) {
						vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
					}
					loaderInfo.vertexPos++;
				}

				hasTangent = bufferTangents && bufferNormals;
			}

			// Indices
			if (hasIndices)
			{
				const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				switch (accessor.componentType)
				{
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
					{
						const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
							loaderInfo.indexPos++;
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
					{
						const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
							loaderInfo.indexPos++;
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
					{
						const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
							loaderInfo.indexPos++;
						}
						break;
					}
					default:
					{
						KLog::Logger->Log(LL_ERROR, "Index component type %d not supported!", accessor.componentType);
						return;
					}
				}
			}

#define CALCULATE_TANGENT_BY_UV 1
#if CALCULATE_TANGENT_BY_UV
			// Calculate tbn by normal and uv0 https://www.bilibili.com/read/cv6012696
			if (!hasTangent)
			{
				std::vector<glm::vec3> normals;
				std::vector<glm::vec3> tangents;
				std::vector<glm::vec3> bitangents;
				std::vector<uint32_t> counters;

				normals.resize(vertexCount);
				tangents.resize(vertexCount);
				bitangents.resize(vertexCount);
				counters.resize(vertexCount);

				std::vector<uint32_t> fakeIndexBuffer;
				if (!hasIndices)
				{
					assert(vertexCount % 3 == 0);
					fakeIndexBuffer.resize(vertexCount);
					for (uint32_t i = 0; i < vertexCount; ++i)
					{
						fakeIndexBuffer[i] = vertexStart + i;
					}
				}

				std::vector<Vertex>& vertexBuffer = loaderInfo.vertexBuffer;
				const std::vector<uint32_t>& indexBuffer = hasIndices ? loaderInfo.indexBuffer : fakeIndexBuffer;

				assert(indexCount % 3 == 0);

				for (uint32_t tri = 0; tri < indexCount / 3; ++tri)
				{
					for (uint32_t idx = 0; idx < 3; ++idx)
					{
						uint32_t idx0 = indexBuffer[indexStart + tri * 3 + idx];
						uint32_t idx1 = indexBuffer[indexStart + tri * 3 + (idx + 1) % 3];
						uint32_t idx2 = indexBuffer[indexStart + tri * 3 + (idx + 2) % 3];

						uint32_t arrayIdx = idx0 - vertexStart;

						glm::vec2 uv0 = vertexBuffer[idx0].uv0;// - glm::floor(vertexBuffer[idx0].uv0);
						glm::vec2 uv1 = vertexBuffer[idx1].uv0;// - glm::floor(vertexBuffer[idx1].uv0);
						glm::vec2 uv2 = vertexBuffer[idx2].uv0;// - glm::floor(vertexBuffer[idx2].uv0);

						glm::vec3 e1 = vertexBuffer[idx1].pos - vertexBuffer[idx0].pos;
						glm::vec3 e2 = vertexBuffer[idx2].pos - vertexBuffer[idx0].pos;

						float delta_u1 = uv1[0] - uv0[0];
						float delta_u2 = uv2[0] - uv0[0];
						float delta_v1 = uv1[1] - uv0[1];
						float delta_v2 = uv2[1] - uv0[1];

						glm::vec3 tangent, bitangent, normal;

						// We have delta uv
						if (abs(delta_v1 * delta_u2 - delta_v2 * delta_u1) > 1e-6f)
						{
							tangent = (delta_v1 * e2 - delta_v2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
							bitangent = (-delta_u1 * e2 + delta_u2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
						}

						bool needRandomTangent = false;

						if (glm::length(tangent) > 1e-6f && glm::length(bitangent) > 1e-6f)
						{
							tangent = glm::normalize(tangent);
							bitangent = glm::normalize(bitangent);

							float dotProduct = glm::dot(tangent, bitangent);
							if (abs(dotProduct) > 0.95f)
							{
								needRandomTangent = true;
							}
						}
						else
						{
							needRandomTangent = true;
						}

						if (needRandomTangent)
						{
							// Keep the original normal
							normal = vertexBuffer[idx0].normal;

							tangent = glm::vec3(1.0f, 0.0f, 0.0f);
							if (abs(glm::dot(normal, tangent) > 1.0f - 1e-2f))
							{
								tangent = glm::vec3(0.0f, 1.0f, 0.0f);
							}

							tangent = glm::normalize(glm::cross(normal, tangent));
							bitangent = glm::normalize(glm::cross(normal, tangent));
						}
						else
						{
							normal = glm::normalize(glm::cross(tangent, bitangent));
						}

						// https://blog.csdn.net/n5/article/details/104215400
						float cosine = glm::dot(glm::normalize(e1), glm::normalize(e2));
						float areaWeight = glm::length(e1) * glm::length(e2) * sqrtf(1.0f - cosine * cosine);
						float angleWeight = acosf(cosine);
						float weight = areaWeight * angleWeight;

						normals[arrayIdx] += normal * weight;
						tangents[arrayIdx] += tangent * weight;
						bitangents[arrayIdx] += bitangent * weight;
						counters[arrayIdx] += 1;
					}
				}

				assert(normals.size() > 0);

				for (size_t i = 0; i < normals.size(); ++i)
				{
					assert(counters[i] > 0);
					tangents[i] = glm::normalize(tangents[i]);
					bitangents[i] = glm::normalize(bitangents[i]);
					normals[i] = glm::normalize(normals[i]);
				}

				for (uint32_t i = 0; i < vertexCount; ++i)
				{
					if (glm::dot(normals[i], vertexBuffer[i + vertexStart].normal) > 0.0f)
					{
						vertexBuffer[i + vertexStart].tangent = tangents[i];
						vertexBuffer[i + vertexStart].binormal = bitangents[i];
						vertexBuffer[i + vertexStart].normal = normals[i];
					}
				}
			}
#endif
			PrimitivePtr newPrimitive = PrimitivePtr(new Primitive(
				indexStart, indexCount,
				vertexStart, vertexCount,
				primitive.material > -1 ? m_Materials[primitive.material] : m_Materials.back())
			);

			newPrimitive->SetBoundingBox(posMin, posMax);
			newMesh->primitives.push_back(newPrimitive);
		}

		// Mesh BB from BBs of primitives
		for (auto p : newMesh->primitives)
		{
			if (p->bb.IsDefault() && !newMesh->bb.IsDefault())
			{
				newMesh->bb = p->bb;
			}
			KAABBBox merge;
			newMesh->bb.Merge(p->bb, merge);
			newMesh->bb = merge;
		}
		newNode->mesh = newMesh;
	}

	if (!parent)
	{
		m_Nodes.push_back(newNode);
	}
	else
	{
		parent->children.push_back(newNode);
	}
	m_LinearNodes.push_back(newNode);
}

bool KGLTFLoader::AppendMeshIntoResult(NodePtr node, KAssetImportResult& result)
{
	if (node->mesh)
	{
		for (PrimitivePtr primitive : node->mesh->primitives)
		{
			KAssetImportResult::ModelPart part;

			Material& material = primitive->material;

			part.material.alphaMode = material.alphaMode;

			part.material.alphaCutoff = material.alphaCutoff;
			part.material.metallicFactor = material.metallicFactor;
			part.material.roughnessFactor = material.roughnessFactor;
			part.material.baseColorFactor = material.baseColorFactor;
			part.material.emissiveFactor = material.emissiveFactor;
			part.material.doubleSided = material.doubleSided;

			part.material.metalWorkFlow = material.pbrWorkflows.metallicRoughness;
			
			for (uint32_t i = 0; i < MTS_COUNT; ++i)
			{
				if (i == MTS_BASE_COLOR && material.baseColorTexture)
				{
					part.material.codecs[i] = material.baseColorTexture->codec;
					part.material.samplers[i] = material.baseColorTexture->sampler;
				}
				if (i == MTS_NORMAL && material.normalTexture)
				{
					part.material.codecs[i] = material.normalTexture->codec;
					part.material.samplers[i] = material.normalTexture->sampler;
				}
				if (i == MTS_SPECULAR_GLOSINESS && material.extension.specularGlossinessTexture)
				{
					part.material.codecs[i] = material.extension.specularGlossinessTexture->codec;
					part.material.samplers[i] = material.extension.specularGlossinessTexture->sampler;
				}
				if (i == MTS_METAL_ROUGHNESS && material.metallicRoughnessTexture)
				{
					part.material.codecs[i] = material.metallicRoughnessTexture->codec;
					part.material.samplers[i] = material.metallicRoughnessTexture->sampler;
				}
				if (i == MTS_EMISSIVE && material.emissiveTexture)
				{
					part.material.codecs[i] = material.emissiveTexture->codec;
					part.material.samplers[i] = material.emissiveTexture->sampler;
				}
				if (i == MTS_AMBIENT_OCCLUSION && material.occlusionTexture)
				{
					part.material.codecs[i] = material.occlusionTexture->codec;
					part.material.samplers[i] = material.occlusionTexture->sampler;
				}
			}

			part.indexBase = primitive->firstIndex;
			part.indexCount = primitive->indexCount;
			part.vertexBase = primitive->firstvertex;
			part.vertexCount = primitive->vertexCount;

			result.parts.push_back(part);
		}

		for (uint32_t i = 0; i < 3; ++i)
		{
			result.extend.min[i] = std::min(result.extend.min[i], node->mesh->bb.GetMin()[i]);
			result.extend.max[i] = std::max(result.extend.max[i], node->mesh->bb.GetMax()[i]);
		}
	}

	for (NodePtr childNode : node->children)
	{
		AppendMeshIntoResult(childNode, result);
	}

	return true;
}

bool KGLTFLoader::ConvertIntoResult(const KAssetImportOption& importOption, KAssetImportResult& result)
{
	result = KAssetImportResult();

	const float* scale = importOption.scale;
	const float* center = importOption.center;
	const float* uvScale = importOption.uvScale;

	const std::vector<Vertex>& loaderVertexBuffer = loaderInfo.vertexBuffer;

	result.vertexCount = (uint32_t)loaderVertexBuffer.size();
	result.verticesDatas.resize(importOption.components.size());

	for (size_t comIdx = 0; comIdx < importOption.components.size(); ++comIdx)
	{
		const KAssetImportOption::ComponentGroup& componentGroup = importOption.components[comIdx];
		KAssetImportResult::VertexDataBuffer& vertexData = result.verticesDatas[comIdx];

		std::vector<float> vertexBuffer;

		for (size_t vertIdx = 0; vertIdx < loaderVertexBuffer.size(); ++vertIdx)
		{
			const Vertex& vertex = loaderVertexBuffer[vertIdx];

			for (const AssetVertexComponent& component : componentGroup)
			{
				switch (component)
				{
					case AVC_POSITION_3F:
						vertexBuffer.push_back(vertex.pos[0] * scale[0] + center[0]);
						vertexBuffer.push_back(vertex.pos[1] * scale[1] + center[1]);
						vertexBuffer.push_back(vertex.pos[2] * scale[2] + center[2]);
						break;
					case AVC_NORMAL_3F:
						vertexBuffer.push_back(vertex.normal[0]);
						vertexBuffer.push_back(vertex.normal[1]);
						vertexBuffer.push_back(vertex.normal[2]);
						break;
					case AVC_UV_2F:
						vertexBuffer.push_back(vertex.uv0[0] * uvScale[0]);
						vertexBuffer.push_back(vertex.uv0[1] * uvScale[1]);
						break;
					case AVC_UV2_2F:
						vertexBuffer.push_back(vertex.uv1[0] * uvScale[0]);
						vertexBuffer.push_back(vertex.uv1[1] * uvScale[1]);
						break;
					case AVC_DIFFUSE_3F:
						vertexBuffer.push_back(vertex.color0[0]);
						vertexBuffer.push_back(vertex.color0[1]);
						vertexBuffer.push_back(vertex.color0[2]);
						break;
					case AVC_SPECULAR_3F:
						vertexBuffer.push_back(vertex.color1[0]);
						vertexBuffer.push_back(vertex.color1[1]);
						vertexBuffer.push_back(vertex.color1[2]);
						break;
					case AVC_TANGENT_3F:
						vertexBuffer.push_back(vertex.tangent[0]);
						vertexBuffer.push_back(vertex.tangent[1]);
						vertexBuffer.push_back(vertex.tangent[2]);
						break;
					case AVC_BINORMAL_3F:
						vertexBuffer.push_back(vertex.binormal[0]);
						vertexBuffer.push_back(vertex.binormal[1]);
						vertexBuffer.push_back(vertex.binormal[2]);
						break;
					default:
						assert(false && "unknown format");
						break;
				}
			}
		}

		vertexData.resize(vertexBuffer.size() * sizeof(vertexBuffer[0]));
		memcpy(vertexData.data(), vertexBuffer.data(), vertexBuffer.size() * sizeof(vertexBuffer[0]));
	}

	result.indexCount = (uint32_t)loaderInfo.indexBuffer.size();
	if (result.indexCount >= 65536)
	{
		result.index16Bit = false;
		result.indicesData.resize(result.indexCount * sizeof(loaderInfo.indexBuffer[0]));
		memcpy(result.indicesData.data(), loaderInfo.indexBuffer.data(), result.indexCount * sizeof(loaderInfo.indexBuffer[0]));
	}
	else
	{
		result.index16Bit = true;
		std::vector<uint16_t> indices;
		indices.resize(result.indexCount);
		for (uint32_t i = 0; i < result.indexCount; ++i)
		{
			indices[i] = (uint16_t)loaderInfo.indexBuffer[i];
		}
		result.indicesData.resize(result.indexCount * sizeof(indices[0]));
		memcpy(result.indicesData.data(), indices.data(), result.indexCount * sizeof(indices[0]));
	}	

	for (NodePtr node : m_Nodes)
	{
		AppendMeshIntoResult(node, result);
	}

	return true;
}

bool KGLTFLoader::Import(const char* pszFile, const KAssetImportOption& importOption, KAssetImportResult& result)
{
	tinygltf::Model gltfModel;
	bool fileLoaded = LoadModel(pszFile, gltfModel);

	if (fileLoaded)
	{
		LoadTextureSamplers(gltfModel);
		LoadTextures(gltfModel);
		LoadMaterials(gltfModel);

		size_t vertexCount = 0;
		size_t indexCount = 0;

		const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

		// Get vertex and index buffer sizes up-front
		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			GetNodeProps(gltfModel.nodes[scene.nodes[i]], gltfModel, vertexCount, indexCount);
		}

		loaderInfo.vertexBuffer.resize(vertexCount);
		loaderInfo.indexBuffer.resize(indexCount);

		// TODO: scene handling with no default scene
		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			LoadNode(nullptr, node, scene.nodes[i], gltfModel, loaderInfo, 1.0f);
		}

		if (gltfModel.animations.size() > 0)
		{
			LoadAnimations(gltfModel);
		}
		LoadSkins(gltfModel);

		ConvertIntoResult(importOption, result);

		return true;
	}
	else
	{
		KLog::Logger->Log(LL_ERROR, "Could not load gltf file [%s] ", pszFile);
		return false;
	}
}