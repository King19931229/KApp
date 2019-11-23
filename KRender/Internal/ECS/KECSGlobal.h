#pragma once
#include "KComponentManager.h"
#include "KEntityManager.h"

namespace KECSGlobal
{
	extern KComponentManager ComponentManager;
	extern KEntityManager EntityManager;

	void Init();
	void UnInit();
}