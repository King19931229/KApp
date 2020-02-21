#pragma once
#include "KComponentBase.h"
#include "Internal/Asset/KMesh.h"

class KRenderComponent : public KComponentBase
{
protected:
	KMeshPtr m_Mesh;
public:
	KRenderComponent();
	virtual ~KRenderComponent();

	bool Init(const char* path);
	bool InitFromAsset(const char* path);
	bool InitAsBox(const KAABBBox& bound);
	bool UnInit();

	inline KMeshPtr GetMesh() { return m_Mesh; }
};