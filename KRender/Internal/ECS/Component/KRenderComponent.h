#pragma once
#include "KComponentBase.h"
#include "Internal/Asset/KMesh.h"
#include "Internal/KRenderGlobal.h"

class KRenderComponent : public KComponentBase
{
protected:
	KMeshPtr m_Mesh;
public:
	KRenderComponent()
		: KComponentBase(CT_RENDER),
		m_Mesh(nullptr)
	{}
	virtual ~KRenderComponent()
	{
		assert(m_Mesh == nullptr);
		m_Mesh = nullptr;
	}

	inline KMeshPtr GetMesh() { return m_Mesh; }

	bool Init(const char* path)
	{
		return KRenderGlobal::MeshManager.Acquire(path, m_Mesh);
	}

	bool InitFromAsset(const char* path)
	{
		return KRenderGlobal::MeshManager.AcquireFromAsset(path, m_Mesh);
	}

	bool UnInit()
	{
		if(m_Mesh)
		{
			KRenderGlobal::MeshManager.Release(m_Mesh);
			m_Mesh = nullptr;
			return true;
		}
		return false;
	}
};