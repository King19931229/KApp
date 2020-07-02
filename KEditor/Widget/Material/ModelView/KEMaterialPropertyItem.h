#pragma once
#include "KRender/Interface/IKMaterial.h"
#include "Property/KEPropertyBaseView.h"
#include <unordered_set>

class KEMaterialPropertyItem
{
public:
	enum MaterialMemberType
	{
		MATERIAL_MEMBER_TYPE_SHADER_PARAMETER,
		MATERIAL_MEMBER_TYPE_MATERIAL_VALUE,
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
	IKMaterialValue* m_MaterialValue;

	KEMaterialPropertyItem* m_Parent;
	KEMaterialPropertyItem** m_Children;

	KEPropertyBaseView::BasePtr m_PropertyView;
	KEPropertyBaseView::BasePtr CreatePropertyView();
	void RefreshPropertyView();
public:
	KEMaterialPropertyItem(KEMaterialPropertyItem* parent);
	~KEMaterialPropertyItem();

	KEMaterialPropertyItem* GetChild(size_t childIndex);

	void InitAsParameter(IKMaterialParameter* shaderParameter);
	void InitAsValue(IKMaterialValue* value);
	void InitAsBlendMode(IKMaterial* material);

	inline KEMaterialPropertyItem* GetParent() { return m_Parent; }
	inline size_t GetChildCount() const { return m_NumChildren; }
	inline const std::string& GetName() const { return m_Name; }
	inline size_t GetIndex() const { return m_Index; }
	inline MaterialMemberType GetType() const { return m_Type; }
	inline KEPropertyBaseView::BasePtr GetPropertyView() { return m_PropertyView; }
};