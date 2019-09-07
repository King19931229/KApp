#pragma once
#include "Interface/IKConfig.h"
#include <string>

namespace KProcess
{
	EXPORT_DLL bool Wait(const std::string& path, const std::string& args, std::string& output);
}