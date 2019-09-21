#pragma once
#include "Publish/KConfig.h"

namespace KStringParser
{
	EXPORT_DLL bool ParseToBOOL(const char* pStr, bool* pOut, size_t uCount);
	EXPORT_DLL bool ParseToUCHAR(const char* pStr, unsigned char* pOut, size_t uCount);
	EXPORT_DLL bool ParseToCHAR(const char* pStr, char* pOut, size_t uCount);
	EXPORT_DLL bool ParseToUSHORT(const char* pStr, unsigned short* pOut, size_t uCount);
	EXPORT_DLL bool ParseToSHORT(const char* pStr, short* pOut, size_t uCount);
	EXPORT_DLL bool ParseToUINT(const char* pStr, unsigned int* pOut, size_t uCount);
	EXPORT_DLL bool ParseToINT(const char* pStr, int* pOut, size_t uCount);
	EXPORT_DLL bool ParseToULONG(const char* pStr, unsigned long* pOut, size_t uCount);
	EXPORT_DLL bool ParseToLONG(const char* pStr, long* pOut, size_t uCount);
	EXPORT_DLL bool ParseToSIZE_T(const char* pStr, size_t* pOut, size_t uCount);
	EXPORT_DLL bool ParseToFloat(const char* pStr, float* pOut, size_t uCount);
	EXPORT_DLL bool ParseToDouble(const char* pStr, double* pOut, size_t uCount);

	EXPORT_DLL bool ParseFromBOOL(char* pOutStr, size_t uSize, const bool* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromUCHAR(char* pOutStr, size_t uSize, const unsigned char* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromCHAR(char* pOutStr, size_t uSize, const char* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromUSHORT(char* pOutStr, size_t uSize, const unsigned short* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromSHORT(char* pOutStr, size_t uSize, const short* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromUINT(char* pOutStr, size_t uSize, const unsigned int* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromINT(char* pOutStr, size_t uSize, const int* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromULONG(char* pOutStr, size_t uSize, const unsigned long* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromLONG(char* pOutStr, size_t uSize, const long* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromSIZE_T(char* pOutStr, size_t uSize, const size_t* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromFloat(char* pOutStr, size_t uSize, const float* pIn, size_t uCount);
	EXPORT_DLL bool ParseFromDouble(char* pOutStr, size_t uSize, const double* pIn, size_t uCount);
}