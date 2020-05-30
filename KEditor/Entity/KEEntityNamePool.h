#pragma once

#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>

class KEEntityNamePool
{
protected:
	struct NamePoolElement
	{
		std::stack<std::string> m_ReservedNameStack;
		std::unordered_set<std::string> m_AllocatedName;
		std::unordered_set<std::string> m_FreedName;

		std::string prefix;
		size_t m_NameCounter;

		NamePoolElement(const std::string& _prefix)
		{
			prefix = _prefix;
			m_NameCounter = 0;
		}

		std::string AllocName();
		bool FreeName(const std::string& name);
	};
	std::unordered_map<std::string, NamePoolElement> m_Pool;

	bool GetBaseName(const std::string& name, std::string& baseName);
public:
	KEEntityNamePool();
	~KEEntityNamePool();

	bool Init();
	bool UnInit();

	void FreeName(const std::string& name);
	std::string AllocName(const std::string& prefix);
};