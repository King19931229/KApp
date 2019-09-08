#pragma once
#include "Interface/IKConfig.h"
#include <string>

namespace KSystem
{
	EXPORT_DLL bool WaitProcess(const std::string& path, const std::string& args, std::string& output);
}