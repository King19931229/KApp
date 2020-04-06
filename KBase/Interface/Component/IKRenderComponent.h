#pragma once
#include "IKComponentBase.h"
#include "KBase/Publish/KAABBBox.h"

struct IKRenderComponent : public IKComponentBase
{
	IKRenderComponent()
		: IKComponentBase(CT_RENDER)
	{
	}

	virtual ~IKRenderComponent() {}

	virtual bool Init(const char* path) = 0;
	virtual bool InitFromAsset(const char* path) = 0;

	virtual bool GetLocalBound(KAABBBox& bound) const = 0;

	virtual bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const = 0;
	virtual bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const = 0;
};