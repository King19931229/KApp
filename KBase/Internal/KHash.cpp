#include "Publish/KHash.h"
#include "Internal/Hash/KMD5.h"
#include <sstream>

namespace KHash
{
	// http://blog.csdn.net/wusuopubupt/article/details/11479869
	uint32_t Time33(const char* pData, size_t uLen)
	{
		uint32_t hash = 5381; 
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
	uint32_t BKDR(const char* pData, size_t uLen)
	{
		uint32_t seed = 31; // 31 131 1313 13131 131313 etc.. 37
		uint32_t hash = 0; 
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

	std::string MD5(const char* pData, size_t uLen)
	{
		unsigned char decrypt[16] = {0};

		using namespace KMD5;
		MD5_CTX md5;

		MD5Init(&md5); 
		MD5Update(&md5, (unsigned char *)pData, (unsigned int)uLen); 
		MD5Final(&md5, decrypt);

		std::stringstream ss;
		for(int i=0; i< 16; ++i)
		{
			ss << std::hex << (unsigned int)decrypt[i];
		}
		std::string result = ss.str();
		return result;
	}
}