#pragma once
#include "Publish/KConfig.h"
#include <string>
#include <vector>

namespace KStringUtil
{
	EXPORT_DLL bool Split(const std::string& src, const std::string& splitChars, std::vector<std::string>& splitResult);
	EXPORT_DLL bool Strip(const std::string& src, const std::string& stripChars, std::string& result, bool left, bool right);
}