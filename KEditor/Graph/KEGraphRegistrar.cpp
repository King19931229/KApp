#include "KEGraphRegistrar.h"
#include <assert.h>

KEGraphRegistrar::CreateFuncMap KEGraphRegistrar::ms_CreateFuncMap;

bool KEGraphRegistrar::Init()
{
	UnInit();
	return true;
}

bool KEGraphRegistrar::UnInit()
{
	ms_CreateFuncMap.clear();
	return true;
}

bool KEGraphRegistrar::RegisterGraphModel(const QString& name, GraphNodeModelCreateFunc func)
{
	auto it = ms_CreateFuncMap.find(name);
	if (it == ms_CreateFuncMap.end())
	{
		ms_CreateFuncMap[name] = func;
		return true;
	}
	return false;
}

bool KEGraphRegistrar::UnRegisterGraphModel(const QString& name)
{
	auto it = ms_CreateFuncMap.find(name);
	if (it != ms_CreateFuncMap.end())
	{
		ms_CreateFuncMap.erase(it);
	}
	return true;
}

KEGraphNodeModelPtr KEGraphRegistrar::GetNodeModel(const QString& name)
{
	auto it = ms_CreateFuncMap.find(name);
	if (it != ms_CreateFuncMap.end())
	{
		KEGraphNodeModelPtr ret = (it->second)();
		return ret;
	}
	assert(false && "unknown node model");
	return nullptr;
}