#pragma once
#include "KBase/Publish/KConfig.h"
#include <thread>

namespace KSystem
{
	EXPORT_DLL void SetThreadName(std::thread& thread, const std::string& name);
	EXPORT_DLL bool WaitProcess(const std::string& path, const std::string& args, const std::string& workingDirectory, std::string& output);
	EXPORT_DLL bool QueryRegistryKey(const std::string& regKey, const std::string& regValue, std::string& value);
}