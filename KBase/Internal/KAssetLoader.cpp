#include "KAssetLoader.h"
#include "KAssetIOHooker.h"

#include "Interface/IKDataStream.h"
#include "Publish/KFileTool.h"

#include "KBase/Interface/IKLog.h"

namespace KAssetLoaderManager
{
	bool CreateAssetLoader()
	{
		assert(Loader == nullptr);
		if (Loader == nullptr)
		{
			// Change this line to normal if you not want to analyse the import process
			// Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
			Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

			// Create a logger instance for Console Output
			// Assimp::DefaultLogger::create("",severity, aiDefaultLogStream_STDOUT);

			// Create a logger instance for File Output (found in project folder or near .exe)
			Assimp::DefaultLogger::create("assimp_log.txt", severity, aiDefaultLogStream_FILE);

			// Now I am ready for logging my stuff
			Assimp::DefaultLogger::get()->info("this is my info-call");

			Loader = IKAssetLoaderPtr(KNEW KAssetLoader());
		}
		return true;
	}

	bool DestroyAssetLoader()
	{
		assert(Loader != nullptr);
		if (Loader != nullptr)
		{
			// Kill it after the work is done
			Assimp::DefaultLogger::kill();
			Loader = nullptr;
		}
		return true;
	}

	IKAssetLoaderPtr Loader = nullptr;
}

static void LogInfo(const std::string& logString)
{
	KLog::Logger->Log(LL_NORMAL, "%s", logString.c_str());
	// Will add message to File with "info" Tag
	Assimp::DefaultLogger::get()->info(logString.c_str());
}

static void LogError(const std::string& logString)
{
	KLog::Logger->Log(LL_ERROR, "%s", logString.c_str());
	// Will add message to File with "error" Tag
	Assimp::DefaultLogger::get()->error(logString);
}

KAssetLoader::KAssetLoader()
{
	KAssetIOHooker* hooker = KNEW KAssetIOHooker(KFileSystem::Manager.get());
	m_Importer.SetIOHandler(hooker);
}

KAssetLoader::~KAssetLoader()
{
}

#define IMPORT_FLAGS (\
aiProcess_FlipWindingOrder |\
aiProcess_Triangulate |aiProcess_SortByPType |\
aiProcess_PreTransformVertices |\
aiProcess_ValidateDataStructure | aiProcess_FindInvalidData\
)

uint32_t KAssetLoader::GetFlags(const KAssetImportOption& importOption)
{
	uint32_t flag = IMPORT_FLAGS;
	for(const KAssetImportOption::ComponentGroup& group : importOption.components)
	{
		for(const AssetVertexComponent& component : group)
		{
			if(component == AVC_NORMAL_3F)
			{
				flag |= aiProcess_GenSmoothNormals;
			}
			if(component == AVC_TANGENT_3F || component == AVC_BINORMAL_3F)
			{
				flag |= (aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
			}
		}
	}
	return flag;
}

bool KAssetLoader::ImportAiScene(const aiScene* scene, const KAssetImportOption& importOption, KAssetImportResult& result)
{
	if(!scene)
	{
		return false;
	}

	if(importOption.components.empty())
	{
		return false;
	}

	const float* scale = importOption.scale;
	const float* center = importOption.center;
	const float* uvScale = importOption.uvScale;

	auto& extend = result.extend;
	auto& parts = result.parts;
		
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0; 

	std::vector<std::vector<float>> vertexBuffers;
	std::vector<uint32_t> index32Buffer;

	vertexBuffers.resize(importOption.components.size());

	// Load meshes
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = scene->mMeshes[i];
		if (paiMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
		{
			continue;
		}
		if (paiMesh->mNumVertices == 0 || paiMesh->mNumFaces == 0)
		{
			continue;
		}
		if (!paiMesh->HasNormals())
		{
			continue;
		}

		aiString name;
		scene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_NAME, name);

		KAssetImportResult::ModelPart part;

		part.vertexBase = vertexCount;
		part.indexBase = indexCount;

		vertexCount += scene->mMeshes[i]->mNumVertices;

		aiColor3D pDiffuse(0.f, 0.f, 0.f);
		scene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pDiffuse);

		aiColor3D pSpecular(0.f, 0.f, 0.f);
		scene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_SPECULAR, pSpecular);

		aiString diffuseMap;
		if(aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseMap))
		{
			KFileTool::PathJoin(m_AssetFolder, diffuseMap.C_Str(), part.material.textures[MTS_DIFFUSE]);
		}

		aiString emissiveMap;
		if (aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveMap))
		{
			KFileTool::PathJoin(m_AssetFolder, emissiveMap.C_Str(), part.material.textures[MTS_EMISSIVE]);
		}

		aiString aoMap;
		if (aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_AMBIENT, 0, &aoMap))
		{
			KFileTool::PathJoin(m_AssetFolder, aoMap.C_Str(), part.material.textures[MTS_AMBIENT_OCCLUSION]);
		}

		aiString specaulrMap;
		if(aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_SPECULAR, 0, &specaulrMap))
		{
			KFileTool::PathJoin(m_AssetFolder, specaulrMap.C_Str(), part.material.textures[MTS_SPECULAR_GLOSINESS]);
		}
		else if (aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_SHININESS, 0, &specaulrMap))
		{
			KFileTool::PathJoin(m_AssetFolder, specaulrMap.C_Str(), part.material.textures[MTS_SPECULAR_GLOSINESS]);
		}

		aiString normalMap;
		if (aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_NORMALS, 0, &normalMap))
		{
			KFileTool::PathJoin(m_AssetFolder, normalMap.C_Str(), part.material.textures[MTS_NORMAL]);
		}
		else if (aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_DISPLACEMENT, 0, &normalMap))
		{
			KFileTool::PathJoin(m_AssetFolder, normalMap.C_Str(), part.material.textures[MTS_NORMAL]);
		}
		else if (aiReturn_SUCCESS == scene->mMaterials[paiMesh->mMaterialIndex]->GetTexture(aiTextureType_HEIGHT, 0, &normalMap))
		{
			KFileTool::PathJoin(m_AssetFolder, normalMap.C_Str(), part.material.textures[MTS_NORMAL]);
		}

		part.material.metalWorkFlow = false;

		const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

		for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
			const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
			const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
			const aiVector3D* pTexCoord2 = (paiMesh->HasTextureCoords(1)) ? &(paiMesh->mTextureCoords[1][j]) : &Zero3D;
			const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
			const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

			for (size_t comIdx = 0; comIdx < importOption.components.size(); ++comIdx)
			{
				const KAssetImportOption::ComponentGroup& componentGroup = importOption.components[comIdx];
				std::vector<float>& vertexBuffer = vertexBuffers[comIdx];

				for(const AssetVertexComponent &component : componentGroup)
				{
					switch (component)
					{
					case AVC_POSITION_3F:
						vertexBuffer.push_back(pPos->x * scale[0] + center[0]);
						vertexBuffer.push_back(pPos->y * scale[1] + center[1]);
						vertexBuffer.push_back(pPos->z * scale[2] + center[2]);
						break;
					case AVC_NORMAL_3F:
						vertexBuffer.push_back(pNormal->x);
						vertexBuffer.push_back(pNormal->y);
						vertexBuffer.push_back(pNormal->z);
						break;
					case AVC_UV_2F:
						vertexBuffer.push_back(pTexCoord->x * uvScale[0]);
						vertexBuffer.push_back(pTexCoord->y * uvScale[1]);
						break;
					case AVC_UV2_2F:
						vertexBuffer.push_back(pTexCoord2->x * uvScale[0]);
						vertexBuffer.push_back(pTexCoord2->y * uvScale[1]);
						break;
					case AVC_DIFFUSE_3F:
						vertexBuffer.push_back(pDiffuse.r);
						vertexBuffer.push_back(pDiffuse.g);
						vertexBuffer.push_back(pDiffuse.b);
						break;
					case AVC_SPECULAR_3F:
						vertexBuffer.push_back(pSpecular.r);
						vertexBuffer.push_back(pSpecular.g);
						vertexBuffer.push_back(pSpecular.b);
						break;
					case AVC_TANGENT_3F:
						vertexBuffer.push_back(pTangent->x);
						vertexBuffer.push_back(pTangent->y);
						vertexBuffer.push_back(pTangent->z);
						break;
					case AVC_BINORMAL_3F:
						vertexBuffer.push_back(pBiTangent->x);
						vertexBuffer.push_back(pBiTangent->y);
						vertexBuffer.push_back(pBiTangent->z);
						break;
					default:
						assert(false && "unknown format");
					};
				}
			}

			extend.max[0] = std::max(pPos->x, extend.max[0]);
			extend.max[1] = std::max(pPos->y, extend.max[1]);
			extend.max[2] = std::max(pPos->z, extend.max[2]);

			extend.min[0] = std::min(pPos->x, extend.min[0]);
			extend.min[1] = std::min(pPos->y, extend.min[1]);
			extend.min[2] = std::min(pPos->z, extend.min[2]);
		}

		part.vertexCount = paiMesh->mNumVertices;

		uint32_t indexBase = part.vertexBase;

		for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			const aiFace& Face = paiMesh->mFaces[j];
			if (Face.mNumIndices != 3)
				continue;

			uint32_t idx[3] = { 0 };
			bool valid = true;

			for (int i = 0; i < 3; ++i)
			{
				idx[i] = indexBase + Face.mIndices[i];
				if (idx[i] >= vertexCount)
				{
					valid = false;
					break;
				}
			}

			if (!valid)			
				continue;

			for (int i = 0; i < 3; ++i) { index32Buffer.push_back(idx[i]); }
			part.indexCount += 3;
			indexCount += 3;
		}

		if (!part.vertexCount || !part.indexCount)
			continue;

		parts.push_back(std::move(part));
	}

	auto& verticesDatas = result.verticesDatas;
	auto& indicesData = result.indicesData;

	verticesDatas.resize(vertexBuffers.size());
	for(size_t vbIdx = 0; vbIdx < vertexBuffers.size(); ++vbIdx)
	{
		auto& verticesData = verticesDatas[vbIdx];
		const auto& verticesBuffer = vertexBuffers[vbIdx];

		verticesData.resize(verticesBuffer.size() * sizeof(float));
		memcpy(verticesData.data(), verticesBuffer.data(), verticesData.size());
	}

	result.vertexCount = vertexCount;
	result.indexCount = indexCount;

	if(indexCount < 0xFFFF)
	{
		std::vector<uint16_t> index16Buffer;
		index16Buffer.resize(indexCount);
		for(size_t i = 0; i < indexCount; ++i)
		{
			index16Buffer[i] = static_cast<uint16_t>(index32Buffer[i]);
		}
		result.index16Bit = true;
		indicesData.resize(sizeof(uint16_t) * index16Buffer.size());
		memcpy(indicesData.data(), index16Buffer.data(), indicesData.size());
	}
	else
	{
		result.index16Bit = false;
		indicesData.resize(sizeof(uint32_t) * index32Buffer.size());
		memcpy(indicesData.data(), index32Buffer.data(), indicesData.size());
	}

	if (result.vertexCount > 0 && result.indexCount > 0)
	{
		return true;
	}
	return false;
}

bool KAssetLoader::ImportFromMemory(const char* pData, size_t dataSize, const KAssetImportOption& importOption, KAssetImportResult& result)
{
	const aiScene* scene = m_Importer.ReadFileFromMemory(pData, dataSize, GetFlags(importOption));
	if(!scene)
	{
		LogError(m_Importer.GetErrorString());
		return false;
	}

	m_AssetFolder.clear();

	// Now we can access the file's contents.
	if(ImportAiScene(scene, importOption, result))
	{
		LogInfo("Import of scene from memory succeeded.");
		return true;
	}

	LogError("Import of scene from memory failure.");
	return false;
}

bool KAssetLoader::Import(const char* pszFile, const KAssetImportOption& importOption, KAssetImportResult& result)
{
	const aiScene* scene = m_Importer.ReadFile(pszFile, GetFlags(importOption));
	if(!scene)
	{
		LogError(m_Importer.GetErrorString());
		return false;
	}

	if(!KFileTool::ParentFolder(pszFile, m_AssetFolder))
	{
		assert(false && "no asset folder");
		return false;
	}

	// Now we can access the file's contents.
	if(ImportAiScene(scene, importOption, result))
	{
		LogInfo("Import of scene from file succeeded.");
		return true;
	}

	LogError("Import of scene from file failure.");
	return false;
}