#pragma once
#include "Interface/IKRenderConfig.h"
#include <map>

typedef std::map<size_t, IKTexturePtr> KMaterialTextrueBinding;

class KSubMaterial
{
	friend class KMaterial;
protected:
	KMaterial* m_Parent;
	KMaterialTextrueBinding m_Textrues;
public:
	KSubMaterial(KMaterial* parent);
	~KSubMaterial();

	bool Init(const KMaterialTextrueBinding& textures);
	bool UnInit();

	const KMaterialTextrueBinding& GetTextureBinding() const { return m_Textrues; }
};

typedef std::shared_ptr<KSubMaterial> KSubMaterialPtr;