#include "KECSGlobal.h"

namespace KECSGlobal
{
	KRenderComponentManager ComponentManager;
	KEntityManager EntityManager;

	void Init()
	{
		ComponentManager.Init();
		EntityManager.Init();
	}

	void UnInit()
	{
		EntityManager.UnInit();
		ComponentManager.UnInit();
	}
}