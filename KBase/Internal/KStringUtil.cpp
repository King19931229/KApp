#include "Publish/KStringUtil.h"

namespace KStringUtil
{
	bool Split(const std::string& input, const std::string splitChars, std::vector<std::string>& splitResult)
	{
		splitResult.clear();
		if(!input.empty())
		{
			std::string::size_type offset = 0;
			while(offset < input.length())
			{
				std::string::size_type nonSplit = input.find_first_not_of(splitChars, offset);
				std::string::size_type split = input.find_first_of(splitChars, offset);
				// both found
				if(nonSplit != std::string::npos && split != std::string::npos)
				{
					// starts with split
					if(nonSplit > split)
					{
						offset = nonSplit;
						continue;
					}
					splitResult.push_back(input.substr(nonSplit, split - nonSplit));
					offset = split + 1;
				}
				// non split not found. exit
				else if(nonSplit == std::string::npos)
				{
					break;
				}
				// split not found
				else
				{
					splitResult.push_back(input.substr(nonSplit));
					break;
				}
			}
			return true;
		}
		return false;
	}
}