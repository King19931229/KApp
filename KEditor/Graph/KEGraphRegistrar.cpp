#include "KEGraphRegistrar.h"
#include <assert.h>

KEGraphRegistrar::KEGraphRegistrar()
{
}

KEGraphRegistrar::~KEGraphRegistrar()
{
	assert(m_CreateFuncMap.empty());
}

bool KEGraphRegistrar::Init()
{
	UnInit();
	return true;
}

bool KEGraphRegistrar::UnInit()
{
	m_CreateFuncMap.clear();
	return true;
}

bool KEGraphRegistrar::RegisterGraphModel(const QString& name, KEGraphNodeModelCreateFunc func)
{
	auto it = m_CreateFuncMap.find(name);
	if (it == m_CreateFuncMap.end())
	{
		m_CreateFuncMap[name] = func;
		return true;
	}
	return false;
}

bool KEGraphRegistrar::UnRegisterGraphModel(const QString& name)
{
	auto it = m_CreateFuncMap.find(name);
	if (it != m_CreateFuncMap.end())
	{
		m_CreateFuncMap.erase(it);
	}
	return true;
}

void KEGraphRegistrar::VisitModel(ModelVisitFunc func)
{
	for (auto it = m_CreateFuncMap.begin(), itEnd = m_CreateFuncMap.end();
		it != itEnd; ++it)
	{
		func(it->first, it->second);
	}
}

KEGraphNodeModelPtr KEGraphRegistrar::GetNodeModel(const QString& name)
{
	auto it = m_CreateFuncMap.find(name);
	if (it != m_CreateFuncMap.end())
	{
		KEGraphNodeModelPtr ret = (it->second)();
		return ret;
	}
	assert(false && "unknown node model");
	return nullptr;
}