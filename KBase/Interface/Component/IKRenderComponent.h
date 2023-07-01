#pragma once
#include "IKComponentBase.h"
#include "KRender/Interface/IKMaterial.h"
#include "KBase/Publish/KAABBBox.h"
#include "KBase/Publish/KDebugUtility.h"

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

	virtual bool SaveAsMesh(const std::string& path) const = 0;

	virtual bool InitAsMesh(const std::string& mesh, bool async) = 0;
	virtual bool InitAsAsset(const std::string& asset, bool async) = 0;
	virtual bool InitAsUserData(const KMeshRawData& userData, const std::string& label, bool async) = 0;
	virtual bool InitAsDebugUtility(const KDebugUtilityInfo& info) = 0;

	virtual void SetUtilityColor(const glm::vec4& color) = 0;
	virtual const glm::vec4& GetUtilityColor() const = 0;
	virtual bool IsUtility() const = 0;

	virtual bool UnInit() = 0;

	virtual bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as) = 0;
};