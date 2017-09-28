#include "Publish/KHashString.h"
#include "Publish/KHash.h"
#include "Publish/KSpinLock.h"

#include <mutex>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>

namespace KHashString
{
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
	typedef std::unordered_map<size_t, KHashStr> HashStrMap;

	HashStrChunks Chunks;
	HashStrMap StrMap;
	KSpinLock SpinLock;

	bool CreateHashStringTable()
	{
		return true;
	}

	bool DestroyHashStringTable()
	{
		Chunks.swap(HashStrChunks());
		return true;
	}

	KHashStr GetHashString(const char* pszStr)
	{
		if(pszStr)
		{
			size_t uLen = strlen(pszStr);
			if(uLen > HASH_STRING_MAX_LEN)
				return nullptr;

			HashStrChunks::iterator it;

			{
				std::lock_guard<KSpinLock> gurad(SpinLock);
				it = Chunks.end();
				it = std::find_if(Chunks.begin(), Chunks.end(), [&pszStr](KHashStrChunkPtr& pChunk)->bool { return pChunk->InChunk(pszStr); });
				if(it != Chunks.end())
					return pszStr;
			}

			size_t uHash = KHash::BKDR(pszStr, uLen);

			{
				std::lock_guard<KSpinLock> gurad(SpinLock);
				HashStrMap::iterator mapIt = StrMap.find(uHash);
				if(mapIt != StrMap.end())
					return mapIt->second;

				it = Chunks.end();
				it = std::find_if(Chunks.begin(), Chunks.end(), [&uLen](KHashStrChunkPtr& pChunk)->bool { return pChunk->HasSpace(uLen); });

				if(it == Chunks.end())
				{
					Chunks.push_back(KHashStrChunkPtr(new KHashStrChunk));
					it = Chunks.end() - 1;
				}

				KHashStr pRet = nullptr;
				if((*it)->Insert(pszStr, uLen, &pRet))
				{
					StrMap.insert(HashStrMap::value_type(uHash, pRet));
					return pRet;
				}
			}
		}
		return nullptr;
	}
}