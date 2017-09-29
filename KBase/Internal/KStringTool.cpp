#include "Publish/KStringTool.h"

#include <sstream>

namespace KStringTool
{
	template<typename T>
	bool Parse(const char* pStr, T* pOut)
	{
		if(pStr && pOut)
		{
			std::stringstream ss;
			ss.str(pStr);
			ss >> *pOut;
			return true;
		}
		return false;
	}

	bool ParseToBOOL(const char* pStr, bool* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToUCHAR(const char* pStr, unsigned char* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToCHAR(const char* pStr, char* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToUSHORT(const char* pStr, unsigned short* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToSHORT(const char* pStr, short* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToUINT(const char* pStr, unsigned int* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToINT(const char* pStr, int* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToULONG(const char* pStr, unsigned long* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToLONG(const char* pStr, long* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToSIZE_T(const char* pStr, size_t* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToFloat(const char* pStr, float* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}

	bool ParseToDouble(const char* pStr, double* pOut)
	{
		bool bRet = Parse(pStr, pOut);
		return bRet;
	}
}