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

	virtual bool SetMeshPath(const char* path) = 0;
	virtual bool SetAssetPath(const char* path) = 0;
	virtual bool GetPath(std::string& path) const = 0;

	virtual bool SaveAsMesh(const char* path) const = 0;

	virtual bool SetHostVisible(bool hostVisible) = 0;
	virtual bool SetUseMaterialTexture(bool useMaterialTex) = 0;
	virtual bool GetUseMaterialTexture() const = 0;

	virtual bool Init(bool async) = 0;
	virtual bool UnInit() = 0;

	virtual bool SetMaterialPath(const char* path) = 0;
	virtual bool ReloadMaterial() = 0;

	virtual IKMaterialPtr GetMaterial() = 0;
};