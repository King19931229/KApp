#pragma once
#include "KRender/Interface/IKMaterial.h"
#include "Property/KEPropertyBaseView.h"
#include <unordered_set>

class KEMaterialPropertyItem
{
public:
	enum MaterialMemberType
	{
		MATERIAL_MEMBER_TYPE_ROOT,

		MATERIAL_MEMBER_TYPE_MATERIAL_TEXTURE,
		MATERIAL_MEMBER_TYPE_MATERIAL_TEXTURE_SLOT,

		MATERIAL_MEMBER_TYPE_VERTEX_SHADER_PARAMETER,
		MATERIAL_MEMBER_TYPE_FRAGMENT_SHADER_PARAMETER,
		MATERIAL_MEMBER_TYPE_MATERIAL_PARAMETER_VALUE,

		MATERIAL_MEMBER_TYPE_BLEND_MODE,

		MATERIAL_MEMBER_TYPE_NONE
	};
protected:
	std::string m_Name;
	MaterialMemberType m_Type;
	size_t m_Index;
	size_t m_NumChildren;

	IKMaterial* m_Material;
	IKMaterialParameter* m_ShaderParameter;
	IKMaterialTextureBinding* m_TextureBinding;
	IKMaterialValue* m_MaterialValue;

	KEMaterialPropertyItem* m_Parent;
	KEMaterialPropertyItem** m_Children;

	KEPropertyBaseView::BasePtr m_PropertyView;
	KEPropertyBaseView::BasePtr CreatePropertyView();
	void RefreshPropertyView();

	void InitAsTextureBinding(IKMaterial* material, IKMaterialTextureBinding* textureBinding);
	void InitAsTextureBindingSlot(IKMaterial* material, IKMaterialTextureBinding* textureBinding, size_t slot);
	void InitAsParameter(IKMaterial* material, IKMaterialParameter* shaderParameter, bool vsShader);
	void InitAsParameterValue(IKMaterial* material, IKMaterialValue* value);
	void InitAsBlendMode(IKMaterial* material);
public:
	KEMaterialPropertyItem(KEMaterialPropertyItem* parent);
	~KEMaterialPropertyItem();

	KEMaterialPropertyItem* GetChild(size_t childIndex);
	
	void UnInit();

	void InitAsMaterial(IKMaterial* material);

	inline KEMaterialPropertyItem* GetParent() { return m_Parent; }
	inline size_t GetChildCount() const { return m_NumChildren; }
	inline const std::string& GetName() const { return m_Name; }
	inline size_t GetIndex() const { return m_Index; }
	inline MaterialMemberType GetType() const { return m_Type; }
	inline KEPropertyBaseView::BasePtr GetPropertyView() { return m_PropertyView; }
};