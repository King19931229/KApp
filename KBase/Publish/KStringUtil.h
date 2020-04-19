#pragma once
#include "KBase/Publish/KConfig.h"
#include <string>
#include <vector>

namespace KStringUtil
{
	EXPORT_DLL bool Split(const std::string& src, const std::string& splitChars, std::vector<std::string>& splitResult);
	EXPORT_DLL bool Strip(const std::string& src, const std::string& stripChars, std::string& result, bool left, bool right);
	EXPORT_DLL bool StartsWith(const std::string& src, const std::string& chars);
	EXPORT_DLL bool EndsWith(const std::string& src, const std::string& chars);
	EXPORT_DLL bool Upper(const std::string& src, std::string& dest);
	EXPORT_DLL bool Lower(const std::string& src, std::string& dest);
}