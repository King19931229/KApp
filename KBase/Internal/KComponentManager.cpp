#include "KComponentManager.h"
#include <assert.h>

namespace KComponent
{
	IKComponentManagerPtr Manager = nullptr;

	bool CreateManager()
	{
		assert(!Manager);
		if (!Manager)
		{
			Manager = IKComponentManagerPtr(new KComponentManager());
			return true;
		}
		return false;
	}

	bool DestroyManager()
	{
		if (Manager)
		{
			Manager = nullptr;
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

bool KComponentManager::RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair pair)
{
	if (type != CT_UNKNOWN)
	{
		FuncInfo& info = m_CreateDestroyFunc[type];
		if (!info.registered)
		{
			info.registered = true;
			info.funcPair = pair;
			return true;
		}
	}
	return false;
}

bool KComponentManager::UnRegisterFunc(ComponentType type)
{
	if (type != CT_UNKNOWN)
	{
		FuncInfo& info = m_CreateDestroyFunc[type];
		info.registered = false;
		return true;
	}
	return false;
}

bool KComponentManager::UnRegisterAllFunc()
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		m_CreateDestroyFunc[i].registered = false;
	}
	return true;
}

IKComponentBase* KComponentManager::Alloc(ComponentType type)
{
	if (type != CT_UNKNOWN)
	{
		FuncInfo& info = m_CreateDestroyFunc[type];
		if (info.registered)
		{
			IKComponentCreateFunc& createFunc = std::get<0>(info.funcPair);
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
			FuncInfo& info = m_CreateDestroyFunc[type];
			if (info.registered)
			{
				IKComponentDestroyFunc& destroyFunc = std::get<1>(info.funcPair);
				destroyFunc(component);
				return;
			}
		}
	}
	assert(false && "fail to free component");
}