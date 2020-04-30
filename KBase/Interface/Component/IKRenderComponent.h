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

	virtual bool GetLocalBound(KAABBBox& bound) const = 0;

	virtual bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const = 0;
	virtual bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const = 0;

	virtual bool SetPathMesh(const char* path) = 0;
	virtual bool SetPathAsset(const char* path) = 0;
	virtual bool GetPath(std::string& path) const = 0;

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};