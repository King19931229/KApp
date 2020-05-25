#pragma once
#include "KBase/Publish/KConfig.h"
#include <string>

namespace KSystem
{
	EXPORT_DLL bool WaitProcess(const std::string& path, const std::string& args, const std::string& workingDirectory, std::string& output);
	EXPORT_DLL bool QueryRegistryKey(const std::string& regKey, const std::string& regValue, std::string& value);
}