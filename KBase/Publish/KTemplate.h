#pragma once

namespace KTemplate
{
	template<typename T, typename Container>
	bool RegisterCallback(Container& container, T* callback)
	{
		if (callback && std::find(container.begin(), container.end(), callback) == container.end())
		{
			container.push_back(callback);
			return true;
		}
		return false;
	}

	template<typename T, typename Container>
	bool UnRegisterCallback(Container& container, T* callback)
	{
		if (callback)
		{
			auto it = std::find(container.begin(), container.end(), callback);
			if (it != container.end())
			{
				container.erase(it);
				return true;
			}
		}
		return false;
	}
}