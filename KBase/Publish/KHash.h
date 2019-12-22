#pragma once
#include "KBase/Publish/KConfig.h"
#include <string>
#include <cstdint>

namespace KHash
{
	EXPORT_DLL uint32_t Time33(const char* pData, size_t uLen);
	EXPORT_DLL uint32_t BKDR(const char* pData, size_t uLen);
	EXPORT_DLL std::string MD5(const char* pData, size_t uLen);
}