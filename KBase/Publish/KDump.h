#pragma once
#include "Publish/KConfig.h"

namespace KDump
{
	EXPORT_DLL bool Init(const char* pszDumpDir);
	EXPORT_DLL bool UnInit();
}