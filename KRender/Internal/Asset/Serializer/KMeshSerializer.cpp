#include "KMeshSerializer.h"
#include "KMeshSerializerV0.h"
#include "KBase/Interface/IKDataStream.h"
#include "KBase/Interface/IKFileSystem.h"

namespace KMeshSerializer
{
	bool LoadFromFile(IKRenderDevice* device, KMesh* pMesh, const char* path, size_t frameInFlight, size_t renderThreadNum)
	{
		IKDataStreamPtr pData = nullptr;
		if(GFileSystemManager->Open(path, IT_FILEHANDLE, pData))
		{
			// no need to judge version now
			KMeshSerializerV0 reader(device);
			bool bRet = reader.LoadFromStream(pMesh, path, pData, frameInFlight, renderThreadNum);
			pData->Close();
			return bRet;
		}
		return false;
	}

	bool SaveAsFile(KMesh* pMesh, const char* path, MeshSerializerVersion version)
	{
		IKDataStreamPtr pData = GetDataStream(IT_FILEHANDLE);
		if(pData && pData->Open(path, IM_WRITE))
		{
			bool bRet = false;
			switch (version)
			{
			case MSV_VERSION_0_0:
				{
					KMeshSerializerV0 writer(nullptr);
					bRet = writer.SaveToStream(pMesh, pData);
				}
				break;
			default:
				break;
			}
			pData->Close();
			return bRet;
		}
		return false;
	}
}