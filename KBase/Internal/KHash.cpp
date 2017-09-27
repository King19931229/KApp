#include "Publish/KHash.h"

namespace KHash
{
	// http://blog.csdn.net/wusuopubupt/article/details/11479869
	size_t Time33(const char* pData, size_t uLen)
	{
		size_t hash = 5381; 
		/* variant with the hash unrolled eight times */ 
		for (; uLen >= 8; uLen -= 8)
		{ 
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
			hash = ((hash << 5) + hash) + *pData++;
		} 
		switch (uLen)
		{
		case 7: hash = ((hash << 5) + hash) + *pData++; /* fallthrough... */
		case 6: hash = ((hash << 5) + hash) + *pData++; /* fallthrough... */
		case 5: hash = ((hash << 5) + hash) + *pData++; /* fallthrough... */
		case 4: hash = ((hash << 5) + hash) + *pData++; /* fallthrough... */
		case 3: hash = ((hash << 5) + hash) + *pData++; /* fallthrough... */
		case 2: hash = ((hash << 5) + hash) + *pData++; /* fallthrough... */
		case 1: hash = ((hash << 5) + hash) + *pData++; break; 
		case 0: break;
		}
		return hash; 
	}

	// http://blog.csdn.net/wanglx_/article/details/40400693
	size_t BKDR(const char* pData, size_t uLen)
	{
		size_t seed = 31; // 31 131 1313 13131 131313 etc.. 37
		size_t hash = 0; 
		/* variant with the hash unrolled eight times */ 
		for (; uLen >= 8; uLen -= 8)
		{ 
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
			hash = hash * seed + *pData++;
		} 
		switch (uLen)
		{
		case 7: hash = hash * seed + *pData++; /* fallthrough... */
		case 6: hash = hash * seed + *pData++; /* fallthrough... */
		case 5: hash = hash * seed + *pData++; /* fallthrough... */
		case 4: hash = hash * seed + *pData++; /* fallthrough... */
		case 3: hash = hash * seed + *pData++; /* fallthrough... */
		case 2: hash = hash * seed + *pData++; /* fallthrough... */
		case 1: hash = hash * seed + *pData++; break; 
		case 0: break;
		}
		return hash; 
	}
}