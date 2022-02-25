#include "Publish/KStringParser.h"

#include <sstream>
#include <algorithm>

namespace KStringParser
{
	template<typename T>
	bool ParseTo(const char* pStr, T* pOut, size_t uCount)
	{
		if(pStr && pOut && uCount > 0)
		{
			std::stringstream ss;
			ss.str(pStr);
			while(uCount--)
				ss >> *pOut++;
			return true;
		}
		return false;
	}

	bool ParseTo(const char* pStr, bool* pOut, size_t uCount)
	{
		if (pStr && pOut && uCount > 0)
		{
			std::string buf;
			std::stringstream ss;
			ss.str(pStr);
			while (uCount--)
			{
				ss >> buf;
				std::transform(buf.begin(), buf.end(), buf.begin(), [](char ch) { return std::tolower(ch); });
				if (buf == "1" || buf == "true")
					*pOut++ = true;
				else
					*pOut++ = false;
			}
			return true;
		}
		return false;
	}

	template<typename T>
	bool ParseFrom(char* pOutStr, size_t uSize, const T* pIn, size_t uCount)
	{
		if(pOutStr && uSize > 0 && pIn && uCount > 0)
		{
			std::stringstream ss;
			while(uCount--)
			{
				ss << *pIn++;
				if(uCount)
					ss << " ";
			}
			std::string str = ss.str();
			if(str.length() < uSize)
			{
#ifndef _WIN32
				strcpy(pOutStr, str.c_str());
#else
				strcpy_s(pOutStr, uSize, str.c_str());
#endif
				return true;
			}
		}
		return false;
	}

	EXPORT_DLL bool ParseToBOOL(const char* pStr, bool* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToUCHAR(const char* pStr, unsigned char* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToCHAR(const char* pStr, char* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToUSHORT(const char* pStr, unsigned short* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToSHORT(const char* pStr, short* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToUINT(const char* pStr, unsigned int* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToINT(const char* pStr, int* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToULONG(const char* pStr, unsigned long* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToLONG(const char* pStr, long* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToSIZE_T(const char* pStr, size_t* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToFLOAT(const char* pStr, float* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseToDOUBLE(const char* pStr, double* pOut, size_t uCount)
	{
		bool bRet = ParseTo(pStr, pOut, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromBOOL(char* pOutStr, size_t uSize, const bool* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromUCHAR(char* pOutStr, size_t uSize, const unsigned char* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromCHAR(char* pOutStr, size_t uSize, const char* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromUSHORT(char* pOutStr, size_t uSize, const unsigned short* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromSHORT(char* pOutStr, size_t uSize, const short* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromUINT(char* pOutStr, size_t uSize, const unsigned int* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromINT(char* pOutStr, size_t uSize, const int* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromULONG(char* pOutStr, size_t uSize, const unsigned long* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromLONG(char* pOutStr, size_t uSize, const long* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromSIZE_T(char* pOutStr, size_t uSize, const size_t* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromFLOAT(char* pOutStr, size_t uSize, const float* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}

	EXPORT_DLL bool ParseFromDOUBLE(char* pOutStr, size_t uSize, const double* pIn, size_t uCount)
	{
		bool bRet = ParseFrom(pOutStr, uSize, pIn, uCount);
		return bRet;
	}
}