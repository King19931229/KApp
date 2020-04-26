#pragma once
#include "Interface/Component/IKComponentManager.h"

class KComponentManager : public IKComponentManager
{
protected:
	struct FuncInfo
	{
		bool registered;
		IKComponentCreateDestroyFuncPair allocfree;
		IKComponentSaveLoadFuncPair saveload;
		FuncInfo()
		{
			registered = false;
		}
	};
	FuncInfo m_FuncInfo[CT_COUNT];
public:
	KComponentManager();
	~KComponentManager();

	virtual bool RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair allocfree, IKComponentSaveLoadFuncPair saveload);
	virtual bool UnRegisterFunc(ComponentType type);
	virtual bool UnRegisterAllFunc();

	virtual IKComponentBase* Alloc(ComponentType type);
	virtual void Free(IKComponentBase* component);

	virtual bool Save(IKComponentBase* component, IKXMLElementPtr element);
	virtual bool Load(IKComponentBase* component, IKXMLElementPtr element);
};