#pragma once
#include "Publish/KConfig.h"

typedef const char* KHashString;
static const size_t HASH_STRING_MAX_LEN = 260;

EXPORT_DLL bool CreateHashStringTable();
EXPORT_DLL bool DestroyHashStringTable();
EXPORT_DLL KHashString GetHashString(const char* pszFormat, ...);