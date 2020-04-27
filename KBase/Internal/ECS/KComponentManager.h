#pragma once
#include "Interface/Component/IKComponentManager.h"

class KComponentManager : public IKComponentManager
{
protected:
	struct FuncInfo
	{
		bool registered;
		IKComponentCreateDestroyFuncPair allocfree;
		FuncInfo()
		{
			registered = false;
		}
	};
	FuncInfo m_FuncInfo[CT_COUNT];
public:
	KComponentManager();
	~KComponentManager();

	virtual bool RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair allocfree);
	virtual bool UnRegisterFunc(ComponentType type);
	virtual bool UnRegisterAllFunc();

	virtual IKComponentBase* Alloc(ComponentType type);
	virtual void Free(IKComponentBase* component);
};