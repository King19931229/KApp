#pragma once
#include "KBase/Publish/KConfig.h"
#include <string>
#include <cstdint>

namespace KHash
{
	EXPORT_DLL uint32_t Time33(const char* pData, size_t uLen);
	EXPORT_DLL uint32_t BKDR(const char* pData, size_t uLen);
	EXPORT_DLL std::string MD5(const char* pData, size_t uLen);

	template <typename T>
	inline void HashCombine(std::uint16_t& seed, const T& val)
	{
		seed ^= std::hash<T>{}(val)+0x9e37U + (seed << 3) + (seed >> 1);
	}

	template <typename T>
	inline void HashCombine(std::uint32_t& seed, const T& val)
	{
		seed ^= std::hash<T>{}(val)+0x9e3779b9U + (seed << 6) + (seed >> 2);
	}

	template <class T>
	inline void HashCombine(std::uint64_t& seed, const T& val)
	{
		seed ^= std::hash<T>{}(val)+0x9e3779b97f4a7c15LLU + (seed << 12) + (seed >> 4);
	}
}