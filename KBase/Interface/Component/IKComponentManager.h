#pragma once
#include "KBase/Interface/Component/IKComponentBase.h"
#include "KBase/Interface/IKXML.h"
#include <functional>
#include <memory>
#include <tuple>

typedef std::function<IKComponentBase*()> IKComponentCreateFunc;
typedef std::function<void(IKComponentBase*)> IKComponentDestroyFunc;

typedef std::function<bool(IKComponentBase*, IKXMLElementPtr element)> IKComponentSaveFunc;
typedef std::function<bool(IKComponentBase*, IKXMLElementPtr element)> IKComponentLoadFunc;

typedef std::tuple<IKComponentCreateFunc, IKComponentDestroyFunc> IKComponentCreateDestroyFuncPair;
typedef std::tuple<IKComponentSaveFunc, IKComponentLoadFunc> IKComponentSaveLoadFuncPair;

struct IKComponentManager
{
	virtual bool RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair allocfree, IKComponentSaveLoadFuncPair saveload) = 0;
	virtual bool UnRegisterFunc(ComponentType type) = 0;
	virtual bool UnRegisterAllFunc() = 0;

	virtual IKComponentBase* Alloc(ComponentType type) = 0;
	virtual void Free(IKComponentBase* component) = 0;

	virtual bool Save(IKComponentBase* component, IKXMLElementPtr element) = 0;
	virtual bool Load(IKComponentBase* component, IKXMLElementPtr element) = 0;
};

typedef std::unique_ptr<IKComponentManager> IKComponentManagerPtr;

namespace KECS
{
	extern bool CreateComponentManager();
	extern bool DestroyComponentManager();
	extern IKComponentManagerPtr ComponentManager;
}
