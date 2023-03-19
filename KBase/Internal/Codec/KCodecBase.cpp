#include "KCodecBase.h"
#include "Interface/IKDataStream.h"
#include "Interface/IKFileSystem.h"
#include "Publish/KStringUtil.h"
#include "Publish/KFileTool.h"

bool KCodecBase::Codec(const char* pszFile, bool forceAlpha, KCodecResult& result)
{
	IKDataStreamPtr stream = nullptr;

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (!system || !system->Open(pszFile, IT_FILEHANDLE, stream))
	{
		system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
		if (!system || !system->Open(pszFile, IT_FILEHANDLE, stream))
		{
			return false;
		}
	}

	return CodecImpl(stream, forceAlpha, result);
}

bool KCodecBase::Codec(const char* pMemory, size_t size, bool forceAlpha, KCodecResult& result)
{
	IKDataStreamPtr stream = nullptr;
	if (pMemory && size > 0)
	{
		stream = GetDataStream(IT_MEMORY);
		stream->Open(pMemory, size, IM_READ);
	}
	return CodecImpl(stream, forceAlpha, result);
}
