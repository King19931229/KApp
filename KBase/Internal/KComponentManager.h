#include "Interface/Component/IKComponentManager.h"

class KComponentManager : public IKComponentManager
{
protected:
	struct FuncInfo
	{
		bool registered;
		IKComponentCreateDestroyFuncPair funcPair;
		FuncInfo()
		{
			registered = false;
		}
	};
	FuncInfo m_CreateDestroyFunc[CT_COUNT];
public:
	KComponentManager();
	~KComponentManager();

	virtual bool RegisterFunc(ComponentType type, IKComponentCreateDestroyFuncPair pair);
	virtual bool UnRegisterFunc(ComponentType type);
	virtual bool UnRegisterAllFunc();

	virtual IKComponentBase* Alloc(ComponentType type);
	virtual void Free(IKComponentBase* component);
};