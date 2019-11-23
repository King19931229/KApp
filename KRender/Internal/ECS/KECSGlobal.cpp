#include "KECSGlobal.h"

namespace KECSGlobal
{
	KComponentManager ComponentManager;
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