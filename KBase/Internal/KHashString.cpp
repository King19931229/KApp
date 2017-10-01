#include "Publish/KHashString.h"
#include "Publish/KHash.h"

#include <mutex>
#include <vector>
#include <memory>
#include <assert.h>
#include <stdarg.h>
#include <algorithm>
#include <unordered_map>

#ifdef _WIN32
#	define PRINTF_S sprintf_s
#	define VSNPRINTF _vsnprintf
#	pragma warning(disable : 4996)
#else
#	define PRINTF_S snprintf
#	define VSNPRINTF vsnprintf
#endif

static const size_t CHUNK_LEN = 1 << 24;

struct KHashStrChunk
{
	char* pData;
	char* pCurPos;
	size_t uSize;
	size_t uRestSize;

	KHashStrChunk()
	{
		pData = new char[CHUNK_LEN];
		assert(pData);
		memset(pData, 0, sizeof(*pData) * CHUNK_LEN);
		pCurPos = pData;
		uSize = CHUNK_LEN;
		uRestSize = CHUNK_LEN;
	}

	~KHashStrChunk()
	{
		delete[] pData;
		pData = nullptr;
	}

	inline bool InChunk(const char* pStr) const
	{
		bool bRet = pStr >= pData && pStr < pData + CHUNK_LEN;
		return bRet;
	}

	inline bool HasSpace(size_t uLen) const
	{
		bool bRet = uLen + 1 <= uRestSize;
		return bRet;
	}

	bool Insert(const char* pStr, size_t uLen, const char** ppPos)
	{
		if(InChunk(pStr))
			return true;
		if(!HasSpace(uLen))
			return false;
		memcpy(pCurPos, pStr, uLen);
		pCurPos[uLen] = '\0';
		*ppPos = pCurPos;
		pCurPos += uLen + 1;
		uRestSize -= uLen + 1;
		return true;
	}
};

typedef std::shared_ptr<KHashStrChunk> KHashStrChunkPtr;
typedef std::vector<KHashStrChunkPtr> HashStrChunks;
typedef std::unordered_map<size_t, KHashString> HashStrMap;

HashStrChunks g_Chunks;
HashStrMap g_StrMap;
std::mutex g_Lock;

bool CreateHashStringTable()
{
	return true;
}

bool DestroyHashStringTable()
{
	g_Chunks.swap(HashStrChunks());
	return true;
}

KHashString _GetHashString(const char* pszStr)
{
	if(pszStr)
	{
		size_t uLen = strlen(pszStr);
		if(uLen > HASH_STRING_MAX_LEN)
			return nullptr;

		HashStrChunks::iterator it;

		{
			std::lock_guard<decltype(g_Lock)> guard(g_Lock);
			it = g_Chunks.end();
			it = std::find_if(g_Chunks.begin(), g_Chunks.end(), [&pszStr](KHashStrChunkPtr& pChunk)->bool { return pChunk->InChunk(pszStr); });
			if(it != g_Chunks.end())
				return pszStr;
		}

		size_t uHash = KHash::BKDR(pszStr, uLen);

		{
			std::lock_guard<decltype(g_Lock)> guard(g_Lock);
			HashStrMap::iterator mapIt = g_StrMap.find(uHash);
			if(mapIt != g_StrMap.end())
				return mapIt->second;

			it = g_Chunks.end();
			it = std::find_if(g_Chunks.begin(), g_Chunks.end(), [&uLen](KHashStrChunkPtr& pChunk)->bool { return pChunk->HasSpace(uLen); });

			if(it == g_Chunks.end())
			{
				g_Chunks.push_back(KHashStrChunkPtr(new KHashStrChunk));
				it = g_Chunks.end() - 1;
			}

			KHashString pRet = nullptr;
			if((*it)->Insert(pszStr, uLen, &pRet))
			{
				g_StrMap.insert(HashStrMap::value_type(uHash, pRet));
				return pRet;
			}
		}
	}
	return nullptr;
}

KHashString GetHashString(const char* pszFormat, ...)
{
	KHashString pRet = nullptr;
	va_list list;
	va_start(list, pszFormat);
	char szBuffer[2048]; szBuffer[0] = '\0';
	VSNPRINTF(szBuffer, sizeof(szBuffer), pszFormat, list);
	pRet = _GetHashString(szBuffer);
	va_end(list);
	return pRet;
}