#pragma once
#include "Interface/IKConfig.h"
#include <string>
#include <vector>

namespace KStringUtil
{
	EXPORT_DLL bool Split(const std::string& src, const std::string splitChars, std::vector<std::string>& splitResult);
}