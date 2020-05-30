#include "KEEntityNamePool.h"
#include "KEditorGlobal.h"

KEEntityNamePool::KEEntityNamePool()
{
}

KEEntityNamePool::~KEEntityNamePool()
{
	ASSERT_RESULT(m_Pool.empty());
}

bool KEEntityNamePool::Init()
{
	UnInit();
	return true;
}

bool KEEntityNamePool::UnInit()
{
	m_Pool.clear();
	return true;
}

std::string KEEntityNamePool::NamePoolElement::AllocName()
{
	assert(m_FreedName.size() == m_ReservedNameStack.size());
	if (!m_ReservedNameStack.empty())
	{
		std::string oldName = m_ReservedNameStack.top();
		m_ReservedNameStack.pop();

		ASSERT_RESULT(m_FreedName.erase(oldName));
		ASSERT_RESULT(m_AllocatedName.insert(oldName).second);

		return oldName;
	}
	else
	{
		std::string newName = prefix + "_" + std::to_string(m_NameCounter++);
		ASSERT_RESULT(m_AllocatedName.insert(newName).second);
		return newName;
	}
}

bool KEEntityNamePool::NamePoolElement::FreeName(const std::string& name)
{
	auto it = m_AllocatedName.find(name);
	if (it != m_AllocatedName.end())
	{
		bool notInFreeName = m_FreedName.find(name) == m_FreedName.end();
		ASSERT_RESULT(notInFreeName);
		if (notInFreeName)
		{
			ASSERT_RESULT(m_FreedName.insert(name).second);
			m_ReservedNameStack.push(name);
			assert(m_FreedName.size() == m_ReservedNameStack.size());
		}
		m_AllocatedName.erase(it);
		return true;
	}
	return false;
}

bool KEEntityNamePool::GetBaseName(const std::string& name, std::string& baseName)
{
	auto pos = name.find_last_of('_');
	if (pos == std::string::npos || pos == name.length() - 1)
	{
		return false;
	}
	baseName = name.substr(0, pos);
	return true;
}

void KEEntityNamePool::FreeName(const std::string& name)
{
	std::string baseName;
	if (GetBaseName(name, baseName))
	{
		auto poolIt = m_Pool.find(baseName);
		if (poolIt != m_Pool.end())
		{
			NamePoolElement& element = poolIt->second;
			element.FreeName(name);
		}
	}
}

std::string KEEntityNamePool::AllocName(const std::string& prefix)
{
	auto poolIt = m_Pool.find(prefix);
	if (poolIt == m_Pool.end())
	{
		poolIt = m_Pool.insert({ prefix, NamePoolElement(prefix) }).first;
	}
	NamePoolElement& element = poolIt->second;
	return element.AllocName();
}