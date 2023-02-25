#pragma once
#include "IKComponentBase.h"
#include "KRender/Interface/IKMaterial.h"
#include "KBase/Publish/KAABBBox.h"

struct IKRenderComponent : public IKComponentBase
{
	RTTR_ENABLE(IKComponentBase)
	RTTR_REGISTRATION_FRIEND
public:
	IKRenderComponent()
		: IKComponentBase(CT_RENDER)
	{
	}

	virtual ~IKRenderComponent() {}

	virtual bool GetLocalBound(KAABBBox& bound) const = 0;

	virtual bool Pick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const = 0;
	virtual bool CloestPick(const glm::vec3& localOrigin, const glm::vec3& localDir, glm::vec3& result) const = 0;

	virtual bool GetPath(std::string& path) const = 0;
	virtual bool GetHostVisible(bool& hostVisible) const = 0;

	virtual bool SaveAsMesh(const std::string& path) const = 0;

	virtual bool InitAsMesh(const std::string& mesh, bool hostVisible, bool async) = 0;
	virtual bool InitAsAsset(const std::string& asset, bool hostVisible, bool async) = 0;
	virtual bool UnInit() = 0;

	virtual bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as) = 0;

	virtual bool IsUtility() const = 0;
};