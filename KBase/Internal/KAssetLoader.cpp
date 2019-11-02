#include "KAssetLoader.h"
#include "Interface/IKDataStream.h"

EXPORT_DLL bool InitAssetLoaderManager()
{
	 // Change this line to normal if you not want to analyse the import process
	//Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
	Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

	// Create a logger instance for Console Output
	Assimp::DefaultLogger::create("",severity, aiDefaultLogStream_STDOUT);

	// Create a logger instance for File Output (found in project folder or near .exe)
	Assimp::DefaultLogger::create("assimp_log.txt",severity, aiDefaultLogStream_FILE);

	// Now I am ready for logging my stuff
	Assimp::DefaultLogger::get()->info("this is my info-call");

	return true;
}

EXPORT_DLL bool UnInitAssetLoaderManager()
{
	// Kill it after the work is done
	Assimp::DefaultLogger::kill();

	return true;
}

static void LogInfo(const std::string& logString)
{
	// Will add message to File with "info" Tag
	Assimp::DefaultLogger::get()->info(logString.c_str());
}

static void LogDebug(const char* logString)
{
	// Will add message to File with "debug" Tag
	Assimp::DefaultLogger::get()->debug(logString);
}

EXPORT_DLL IKAssetLoaderPtr GetAssetLoader()
{
	return IKAssetLoaderPtr(new KAssetLoader());
}

#define IMPORT_FLAGS (aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals)

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
	parts.resize(scene->mNumMeshes);
		
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0; 

	std::vector<float> vertexBuffer;
	std::vector<uint32_t> index32Buffer;

	// Load meshes
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = scene->mMeshes[i];

		parts[i].vertexBase = vertexCount;
		parts[i].indexBase = indexCount;

		vertexCount += scene->mMeshes[i]->mNumVertices;

		aiColor3D pColor(0.f, 0.f, 0.f);
		scene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

		const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

		for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
			const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
			const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
			const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
			const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

			for (const AssetVertexComponent& component: importOption.components)
			{
				switch (component)
				{
				case AVC_POSITION_3F:
					vertexBuffer.push_back(pPos->x * scale[0] + center[0]);
					vertexBuffer.push_back(-pPos->y * scale[1] + center[1]);
					vertexBuffer.push_back(pPos->z * scale[2] + center[2]);
					break;
				case AVC_NORMAL_3F:
					vertexBuffer.push_back(pNormal->x);
					vertexBuffer.push_back(-pNormal->y);
					vertexBuffer.push_back(pNormal->z);
					break;
				case AVC_UV_2F:
					vertexBuffer.push_back(pTexCoord->x * uvScale[0]);
					vertexBuffer.push_back(pTexCoord->y * uvScale[1]);
					break;
				case AVC_COLOR_3F:
					vertexBuffer.push_back(pColor.r);
					vertexBuffer.push_back(pColor.g);
					vertexBuffer.push_back(pColor.b);
					break;
				case AVC_TANGENT_3F:
					vertexBuffer.push_back(pTangent->x);
					vertexBuffer.push_back(pTangent->y);
					vertexBuffer.push_back(pTangent->z);
					break;
				case AVC_BITANGENT_3F:
					vertexBuffer.push_back(pBiTangent->x);
					vertexBuffer.push_back(pBiTangent->y);
					vertexBuffer.push_back(pBiTangent->z);
					break;
				default:
					assert(false && "unknown format");
				};
			}

			extend.max[0] = std::max(pPos->x, extend.max[0]);
			extend.max[1] = std::max(pPos->y, extend.max[1]);
			extend.max[2] = std::max(pPos->z, extend.max[2]);

			extend.min[0] = std::min(pPos->x, extend.min[0]);
			extend.min[1] = std::min(pPos->y, extend.min[1]);
			extend.min[2] = std::min(pPos->z, extend.min[2]);
		}

		parts[i].vertexCount = paiMesh->mNumVertices;

		uint32_t indexBase = static_cast<uint32_t>(index32Buffer.size());
		for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			const aiFace& Face = paiMesh->mFaces[j];
			if (Face.mNumIndices != 3)
				continue;
			index32Buffer.push_back(indexBase + Face.mIndices[0]);
			index32Buffer.push_back(indexBase + Face.mIndices[1]);
			index32Buffer.push_back(indexBase + Face.mIndices[2]);
			parts[i].indexCount += 3;
			indexCount += 3;
		}
	}

	auto& verticesData = result.verticesData;
	auto& indicesData = result.indicesData;

	verticesData.resize(sizeof(float) * vertexBuffer.size());
	memcpy(verticesData.data(), vertexBuffer.data(), verticesData.size());

	if(indexCount < 0xFFFF)
	{
		std::vector<uint16_t> index16Buffer;
		index16Buffer.resize(indexCount);
		for(size_t i = 0; i < indexCount; ++i)
		{
			index16Buffer[i] = static_cast<uint16_t>(index32Buffer[i]);
		}

		indicesData.resize(sizeof(uint16_t) * index16Buffer.size());
		memcpy(indicesData.data(), index16Buffer.data(), indicesData.size());
	}
	else
	{
		indicesData.resize(sizeof(uint16_t) * index32Buffer.size());
		memcpy(indicesData.data(), index32Buffer.data(), indicesData.size());
	}

	return true;
}

bool KAssetLoader::ImportFromMemory(const char* pData, size_t dataSize, const KAssetImportOption& importOption, KAssetImportResult& result)
{
	const aiScene* scene = m_Importer.ReadFileFromMemory(pData, dataSize, IMPORT_FLAGS);
	if(!scene)
	{
		LogInfo(m_Importer.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	if(ImportAiScene(scene, importOption, result))
	{
		LogInfo("Import of scene from memory succeeded.");
		return true;
	}

	return false;
}

bool KAssetLoader::Import(const char* pszFile, const KAssetImportOption& importOption, KAssetImportResult& result)
{
	const aiScene* scene = m_Importer.ReadFile(pszFile, IMPORT_FLAGS);
	if(!scene)
	{
		LogInfo(m_Importer.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	if(ImportAiScene(scene, importOption, result))
	{
		LogInfo("Import of scene from memory succeeded.");
		return true;
	}

	return false;
}