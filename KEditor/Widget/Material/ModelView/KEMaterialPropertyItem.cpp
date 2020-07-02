#include "KEMaterialPropertyItem.h"

KEMaterialPropertyItem::KEMaterialPropertyItem(KEMaterialPropertyItem* parent)
	: m_Type(MATERIAL_MEMBER_TYPE_NONE),
	m_Index(0),
	m_NumChildren(0),
	m_Material(nullptr),
	m_ShaderParameter(nullptr),
	m_MaterialValue(nullptr),
	m_Parent(parent),
	m_Children(nullptr)
{
}

KEMaterialPropertyItem::~KEMaterialPropertyItem()
{
	if (m_Children)
	{
		for (size_t i = 0; i < m_NumChildren; ++i)
		{
			SAFE_DELETE(m_Children[i]);
		}
		SAFE_DELETE_ARRAY(m_Children);
	}
}

KEPropertyBaseView::BasePtr KEMaterialPropertyItem::CreatePropertyView()
{
	return nullptr;
}

void KEMaterialPropertyItem::RefreshPropertyView()
{

}

KEMaterialPropertyItem* KEMaterialPropertyItem::GetChild(size_t childIndex)
{
	if (childIndex < m_NumChildren)
	{
		return m_Children[childIndex];
	}
	return nullptr;
}

void KEMaterialPropertyItem::InitAsParameter(IKMaterialParameter* shaderParameter)
{
	m_ShaderParameter = shaderParameter;
	const std::vector<IKMaterialValuePtr>& values = shaderParameter->GetAllValues();

	m_Type = MATERIAL_MEMBER_TYPE_SHADER_PARAMETER;
	m_NumChildren = values.size();
	m_Children = KNEW KEMaterialPropertyItem* [m_NumChildren];

	for (size_t i = 0; i < m_NumChildren; ++i)
	{
		IKMaterialValuePtr value = values[i];
		m_Children[i] = KNEW KEMaterialPropertyItem(this);
		m_Children[i]->InitAsValue(value.get());
	}
}

void KEMaterialPropertyItem::InitAsValue(IKMaterialValue* value)
{
	m_Type = MATERIAL_MEMBER_TYPE_MATERIAL_VALUE;
	m_MaterialValue = value;
	m_PropertyView = CreatePropertyView();
	RefreshPropertyView();
}

void KEMaterialPropertyItem::InitAsBlendMode(IKMaterial* material)
{
	m_Type = MATERIAL_MEMBER_TYPE_BLEND_MODE;
	m_Material = material;
	m_PropertyView = CreatePropertyView();
}