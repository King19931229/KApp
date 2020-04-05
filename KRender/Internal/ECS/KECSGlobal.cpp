#include "KECSGlobal.h"

namespace KECSGlobal
{
	KRenderComponentManager ComponentManager;

	void Init()
	{
		ComponentManager.Init();
	}

	void UnInit()
	{
		ComponentManager.UnInit();
	}
}