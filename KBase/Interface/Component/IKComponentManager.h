#pragma once
#include "KBase/Interface/Component/IKComponentBase.h"
#include "KBase/Interface/IKXML.h"
#include <functional>
#include <memory>
#include <tuple>
#include <assert.h>

typedef std::function<IKComponentBase*()> IKComponentCreateFunc;
typedef std::function<void(IKComponentBase*)> IKComponentDestroyFunc;

typedef std::tuple<IKComponentCreateFunc, IKComponentDestroyFunc> IKComponentCreateDestroyFuncPair;

struct IKComponentManager
{
	virtual ~IKComponentManager() {}
	virtual bool RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair allocfree) = 0;
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

	inline const char* ComponentTypeToString(ComponentType componentType)
	{
#define ENUM(type) case CT_##type: return #type;
		switch (componentType)
		{
			ENUM(TRANSFORM);
			ENUM(RENDER);
			ENUM(DEBUG);
		default:
			assert(false);
			return "UNKNOWN";
		}
#undef ENUM
	}

	inline ComponentType StringToComponentType(const char* str)
	{
#define CMP(enum_string) if (!strcmp(str, #enum_string)) return CT_##enum_string;
		CMP(TRANSFORM);
		CMP(RENDER);
		CMP(TRANSFORM);
		return CT_UNKNOWN;
#undef CMP
	}
}
