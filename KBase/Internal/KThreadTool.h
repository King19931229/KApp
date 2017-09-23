#pragma once

#define LIB_EXPORT

#ifdef LIB_EXPORT
#	define EXPORT_DLL
#else
#	ifdef DLL_EXPORT
#		define EXPORT_DLL _declspec(dllexport)
#	else
#		define EXPORT_DLL _declspec(dllimport)
#	endif
#endif

namespace KThreadTool
{
	LIB_EXPORT void SetThreadName(char const* pszName, unsigned long uThreadID = -1);
}