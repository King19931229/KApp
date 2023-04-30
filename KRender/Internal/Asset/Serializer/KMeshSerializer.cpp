#include "KMeshSerializer.h"
#include "KMeshSerializerV0.h"
#include "KBase/Interface/IKDataStream.h"
#include "KBase/Interface/IKFileSystem.h"
#include "Internal/KRenderGlobal.h"

namespace KMeshSerializer
{
	bool LoadFromFile(KMesh* pMesh, const char* path)
	{
		IKDataStreamPtr pData = nullptr;
		IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
		if (!(system && system->Open(path, IT_FILEHANDLE, pData)))
		{
			system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
			if (!(system && system->Open(path, IT_FILEHANDLE, pData)))
			{
				return false;
			}
		}
		// no need to judge version now
		KMeshSerializerV0 reader(KRenderGlobal::RenderDevice);
		bool bRet = reader.LoadFromStream(pMesh, path, pData);
		pData->Close();
		return bRet;
	}

	bool SaveAsFile(const KMesh* pMesh, const char* path, MeshSerializerVersion version)
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