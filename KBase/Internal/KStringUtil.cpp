#include "Publish/KStringUtil.h"
#include <algorithm>

namespace KStringUtil
{
	bool Split(const std::string& input, const std::string& splitChars, std::vector<std::string>& splitResult)
	{
		splitResult.clear();
		if(!input.empty())
		{
			std::string::size_type offset = 0;
			while (offset < input.length())
			{
				std::string::size_type split = input.find_first_of(splitChars, offset);
				if (split != std::string::npos)
				{
					if (!(offset == split && offset == input.length() - 1))
					{
						splitResult.push_back(input.substr(offset, split - offset));
					}
					offset = split + 1;
				}
				else
				{
					splitResult.push_back(input.substr(offset));
					offset = input.length();
				}
			}
			return true;
		}
		return false;
	}

	bool Strip(const std::string& src, const std::string& stripChars, std::string& result, bool left, bool right)
	{
		if(!src.empty())
		{
			std::string::size_type startPos = 0;
			std::string::size_type endPos = src.length() - 1;

			if(left)
			{
				startPos = src.find_first_not_of(stripChars);
			}
			if(right)
			{
				endPos = src.find_last_not_of(stripChars);
			}
			if(startPos != std::string::npos)
			{
				if(endPos != std::string::npos)
				{
					result = src.substr(startPos, endPos - startPos + 1);
				}
				else
				{
					result = src.substr(startPos);
				}
			}
			else
			{
				result = "";
			}
			return true;
		}
		return false;
	}

	bool StartsWith(const std::string& src, const std::string& chars)
	{
		if(src.length() >= chars.length() && src.substr(0, chars.length()) == chars)
			return true;
		return false;
	}

	bool EndsWith(const std::string& src, const std::string& chars)
	{
		if(src.length() >= chars.length() && src.substr(src.length() - chars.length()) == chars)
			return true;
		return false;
	}

	bool Upper(const std::string& src, std::string& dest)
	{
		dest = src;
		std::transform(dest.begin(), dest.end(), dest.begin(), toupper);
		return true;
	}

	bool Lower(const std::string& src, std::string& dest)
	{
		dest = src;
		std::transform(dest.begin(), dest.end(), dest.begin(), tolower);
		return true;
	}
}