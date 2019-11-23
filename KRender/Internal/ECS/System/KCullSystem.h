#pragma once

#include "Internal/ECS/Component/KRenderComponent.h"
#include "Publish/KCamera.h"

struct KCullSystem
{
protected:
public:
	KCullSystem();
	~KCullSystem();
	void Execute(const KCamera& camera, std::vector<KRenderComponent*>& result);
};