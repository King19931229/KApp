#include "KComponentManager.h"
#include <assert.h>

namespace KECS
{
	IKComponentManagerPtr ComponentManager = nullptr;

	bool CreateComponentManager()
	{
		assert(!ComponentManager);
		if (!ComponentManager)
		{
			ComponentManager = IKComponentManagerPtr(new KComponentManager());
			return true;
		}
		return false;
	}

	bool DestroyComponentManager()
	{
		if (ComponentManager)
		{
			ComponentManager = nullptr;
			return true;
		}
		return false;
	}
}

KComponentManager::KComponentManager()
{
}

KComponentManager::~KComponentManager()
{
}

bool KComponentManager::RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair allocfree, IKComponentSaveLoadFuncPair saveload)
{
	if (type != CT_UNKNOWN)
	{
		FuncInfo& info = m_FuncInfo[type];
		if (!info.registered)
		{
			info.registered = true;
			info.allocfree = allocfree;
			info.saveload = saveload;
			return true;
		}
	}
	return false;
}

bool KComponentManager::UnRegisterFunc(ComponentType type)
{
	if (type != CT_UNKNOWN)
	{
		FuncInfo& info = m_FuncInfo[type];
		info.registered = false;
		return true;
	}
	return false;
}

bool KComponentManager::UnRegisterAllFunc()
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		m_FuncInfo[i].registered = false;
	}
	return true;
}

IKComponentBase* KComponentManager::Alloc(ComponentType type)
{
	if (type != CT_UNKNOWN)
	{
		FuncInfo& info = m_FuncInfo[type];
		if (info.registered)
		{
			IKComponentCreateFunc& createFunc = std::get<0>(info.allocfree);
			return createFunc();
		}
	}
	assert(false && "fail to alloc component");
	return nullptr;
}

void KComponentManager::Free(IKComponentBase* component)
{
	if (component)
	{
		ComponentType type = component->GetType();
		if (type != CT_UNKNOWN)
		{
			FuncInfo& info = m_FuncInfo[type];
			if (info.registered)
			{
				IKComponentDestroyFunc& destroyFunc = std::get<1>(info.allocfree);
				destroyFunc(component);
				return;
			}
		}
	}
	assert(false && "fail to free component");
}

bool KComponentManager::Save(IKComponentBase* component, IKXMLElementPtr element)
{
	if (component)
	{
		ComponentType type = component->GetType();
		if (type != CT_UNKNOWN)
		{
			FuncInfo& info = m_FuncInfo[type];
			if (info.registered)
			{
				IKComponentSaveFunc& saveFunc = std::get<0>(info.saveload);
				return saveFunc(component, element);
			}
		}
	}
	assert(false && "fail to save component");
	return false;
}

bool KComponentManager::Load(IKComponentBase* component, IKXMLElementPtr element)
{
	if (component)
	{
		ComponentType type = component->GetType();
		if (type != CT_UNKNOWN)
		{
			FuncInfo& info = m_FuncInfo[type];
			if (info.registered)
			{
				IKComponentSaveFunc& loadFunc = std::get<1>(info.saveload);
				return loadFunc(component, element);
			}
		}
	}
	assert(false && "fail to load component");
	return false;
}