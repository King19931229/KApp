#pragma once
#include "KBase/Interface/Component/IKComponentBase.h"
#include <functional>
#include <memory>
#include <tuple>

typedef std::function<IKComponentBase*()> IKComponentCreateFunc;
typedef std::function<void(IKComponentBase*)> IKComponentDestroyFunc;
typedef std::tuple<IKComponentCreateFunc, IKComponentDestroyFunc> IKComponentCreateDestroyFuncPair;

struct IKComponentManager
{
	virtual bool RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair pair) = 0;
	virtual bool UnRegisterFunc(ComponentType type) = 0;
	virtual bool UnRegisterAllFunc() = 0;

	virtual IKComponentBase* Alloc(ComponentType type) = 0;
	virtual void Free(IKComponentBase* component) = 0;
};

typedef std::unique_ptr<IKComponentManager> IKComponentManagerPtr;

namespace KECS
{
	extern bool CreateComponentManager();
	extern bool DestroyComponentManager();
	extern IKComponentManagerPtr ComponentManager;
}
