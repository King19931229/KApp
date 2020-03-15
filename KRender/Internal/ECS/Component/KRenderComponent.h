#pragma once
#include "KComponentBase.h"
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Utility/KMeshUtilityInfo.h"

class KRenderComponent : public KComponentBase
{
protected:
	KMeshPtr m_Mesh;
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool Init(const char* path);
	bool InitFromAsset(const char* path);
	bool InitUtility(const KMeshUtilityInfoPtr& info);
	bool UnInit();

	bool UpdateUtility(const KMeshUtilityInfoPtr& info);

	inline KMeshPtr GetMesh() { return m_Mesh; }
};