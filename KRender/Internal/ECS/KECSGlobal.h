#pragma once
#include "KRenderComponentManager.h"
#include "KEntityManager.h"

namespace KECSGlobal
{
	extern KRenderComponentManager ComponentManager;
	extern KEntityManager EntityManager;

	void Init();
	void UnInit();
}