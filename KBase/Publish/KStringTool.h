#pragma once
#include "Interface/IKConfig.h"

namespace KStringTool
{
	EXPORT_DLL bool ParseToBOOL(const char* pStr, bool* pOut);
	EXPORT_DLL bool ParseToUCHAR(const char* pStr, unsigned char pOut);
	EXPORT_DLL bool ParseToCHAR(const char* pStr, char* pOut);
	EXPORT_DLL bool ParseToUSHORT(const char* pStr, unsigned short* pOut);
	EXPORT_DLL bool ParseToSHORT(const char* pStr, short* pOut);
	EXPORT_DLL bool ParseToUINT(const char* pStr, unsigned int* pOut);
	EXPORT_DLL bool ParseToINT(const char* pStr, int* pOut);
	EXPORT_DLL bool ParseToULONG(const char* pStr, unsigned long* pOut);
	EXPORT_DLL bool ParseToLONG(const char* pStr, long* pOut);
	EXPORT_DLL bool ParseToSIZE_T(const char* pStr, size_t* pOut);
	EXPORT_DLL bool ParseToFloat(const char* pStr, float* pOut);
	EXPORT_DLL bool ParseToDouble(const char* pStr, double* pOut);
}