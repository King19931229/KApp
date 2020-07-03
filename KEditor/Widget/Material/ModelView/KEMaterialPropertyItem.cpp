#include "KEMaterialPropertyItem.h"

#include "Property/KEPropertyComboView.h"
#include "Property/KEPropertyLineEditView.h"
#include "Property/KEPropertySliderView.h"
#include "Property/KEPropertyCheckBoxView.h"

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
	UnInit();
}

KEPropertyBaseView::BasePtr KEMaterialPropertyItem::CreatePropertyView()
{
	switch (m_Type)
	{
		case MATERIAL_MEMBER_TYPE_MATERIAL_VALUE:
		{
			MaterialValueType valueType = m_MaterialValue->GetType();
			uint8_t vecSize = m_MaterialValue->GetVecSize();

			switch (valueType)
			{
				case MaterialValueType::BOOL:
				{
					bool* value = (bool*)m_MaterialValue->GetData();
					std::shared_ptr<KEPropertyBaseView> boolView = nullptr;
					if (vecSize == 1)
					{
						boolView = KEditor::MakeCheckBoxView<bool, 1>(value);
						boolView->Cast<bool, 1>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
						});
					}
					else if (vecSize == 2)
					{
						boolView = KEditor::MakeCheckBoxView<bool, 2>(value);
						boolView->Cast<bool, 2>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
						});
					}
					else if (vecSize == 3)
					{
						boolView = KEditor::MakeCheckBoxView<bool, 3>(value);
						boolView->Cast<bool, 3>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
							value[2] = newValue[2];
						});
					}
					else if (vecSize == 4)
					{
						boolView = KEditor::MakeCheckBoxView<bool, 4>(value);
						boolView->Cast<bool, 4>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
							value[2] = newValue[2];
							value[3] = newValue[3];
						});
					}
					ASSERT_RESULT(boolView);

					return boolView;
				}
				case MaterialValueType::INT:
				{
					int* value = (int*)m_MaterialValue->GetData();
					std::shared_ptr<KEPropertyBaseView> intView = nullptr;
					if (vecSize == 1)
					{
						intView = KEditor::MakeLineEditView<int, 1>(value);
						intView->Cast<int, 1>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
						});
					}
					else if (vecSize == 2)
					{
						intView = KEditor::MakeLineEditView<int, 2>(value);
						intView->Cast<int, 2>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
						});
					}
					else if (vecSize == 3)
					{
						intView = KEditor::MakeLineEditView<int, 3>(value);
						intView->Cast<int, 3>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
							value[2] = newValue[2];
						});
					}
					else if (vecSize == 4)
					{
						intView = KEditor::MakeLineEditView<int, 4>(value);
						intView->Cast<int, 4>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
							value[2] = newValue[2];
							value[3] = newValue[3];
						});
					}
					ASSERT_RESULT(intView);
					return intView;
				}
				case MaterialValueType::FLOAT:
				{
					float* value = (float*)m_MaterialValue->GetData();
					std::shared_ptr<KEPropertyBaseView> floatView = nullptr;
					if (vecSize == 1)
					{
						floatView = KEditor::MakeLineEditView<float, 1>(value);
						floatView->Cast<float, 1>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
						});
					}
					else if (vecSize == 2)
					{
						floatView = KEditor::MakeLineEditView<float, 2>(value);
						floatView->Cast<float, 2>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
						});
					}
					else if (vecSize == 3)
					{
						floatView = KEditor::MakeLineEditView<float, 3>(value);
						floatView->Cast<float, 3>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
							value[2] = newValue[2];
						});
					}
					else if (vecSize == 4)
					{
						floatView = KEditor::MakeLineEditView<float, 4>(value);
						floatView->Cast<float, 4>()->AddListener([value](auto newValue)
						{
							value[0] = newValue[0];
							value[1] = newValue[1];
							value[2] = newValue[2];
							value[3] = newValue[3];
						});
					}
					ASSERT_RESULT(floatView);
					return floatView;
				}
				case MaterialValueType::UNKNOWN:
				default:
				{
					assert(false && "impossible to reach");
					return nullptr;
					break;
				}
			}
		}
		case MATERIAL_MEMBER_TYPE_BLEND_MODE:
		{
			std::shared_ptr<KEPropertyBaseView> blendView = nullptr;

			blendView = KEditor::MakeComboEditView<uint32_t>();
			blendView->SafeComboCast<uint32_t>()->AppendMapping(OPAQUE, "OPAQUE");
			blendView->SafeComboCast<uint32_t>()->AppendMapping(TRANSRPANT, "TRANSRPANT");
			MaterialBlendMode blendMode = m_Material->GetBlendMode();
			blendView->SafeComboCast<uint32_t>()->SetValue(blendMode);

			blendView->Cast<uint32_t, 1>()->AddListener([this](uint32_t newValue)
			{
				m_Material->SetBlendMode((MaterialBlendMode)newValue);
			});

			return blendView;
		}
	}
	assert(false && "impossible to reach");
	return nullptr;
}

void KEMaterialPropertyItem::RefreshPropertyView()
{
	// TODO
}

KEMaterialPropertyItem* KEMaterialPropertyItem::GetChild(size_t childIndex)
{
	if (childIndex < m_NumChildren)
	{
		return m_Children[childIndex];
	}
	return nullptr;
}

void KEMaterialPropertyItem::UnInit()
{
	m_Type = MATERIAL_MEMBER_TYPE_NONE;
	m_Name.clear();

	if (m_Children)
	{
		for (size_t i = 0; i < m_NumChildren; ++i)
		{
			SAFE_DELETE(m_Children[i]);
		}
		SAFE_DELETE_ARRAY(m_Children);
	}

	m_Material = nullptr;
	m_ShaderParameter = nullptr;
	m_MaterialValue = nullptr;
}

void KEMaterialPropertyItem::InitAsMaterial(IKMaterial* material)
{
	m_Type = MATERIAL_MEMBER_TYPE_ROOT;
	m_Name = "Material";
	m_Material = material;

	KEMaterialPropertyItem* vsParameterItem = KNEW KEMaterialPropertyItem(this);
	vsParameterItem->InitAsParameter(material, material->GetVSParameter().get(), true);

	KEMaterialPropertyItem* fsParameterItem = KNEW KEMaterialPropertyItem(this);
	fsParameterItem->InitAsParameter(material, material->GetFSParameter().get(), false);

	KEMaterialPropertyItem* blendItem = KNEW KEMaterialPropertyItem(this);
	blendItem->InitAsBlendMode(material);

	m_NumChildren = 3;
	m_Children = KNEW KEMaterialPropertyItem*[m_NumChildren];

	m_Children[0] = vsParameterItem;
	m_Children[1] = fsParameterItem;
	m_Children[2] = blendItem;

	for (size_t i = 0; i < m_NumChildren; ++i)
	{
		m_Children[i]->m_Index = i;
	}
}

void KEMaterialPropertyItem::InitAsParameter(IKMaterial* material, IKMaterialParameter* shaderParameter, bool vsShader)
{
	m_Type = vsShader ? MATERIAL_MEMBER_TYPE_VERTEX_SHADER_PARAMETER : MATERIAL_MEMBER_TYPE_FRAGMENT_SHADER_PARAMETER;
	m_Name = vsShader ? "VSParameter" : "FSParameter";
	m_Material = material;
	m_ShaderParameter = shaderParameter;

	const std::vector<IKMaterialValuePtr>& values = shaderParameter->GetAllValues();

	m_NumChildren = values.size();
	m_Children = KNEW KEMaterialPropertyItem*[m_NumChildren];

	for (size_t i = 0; i < m_NumChildren; ++i)
	{
		IKMaterialValuePtr value = values[i];
		m_Children[i] = KNEW KEMaterialPropertyItem(this);
		m_Children[i]->InitAsValue(material, value.get());
		m_Children[i]->m_Index = i;
	}
}

void KEMaterialPropertyItem::InitAsValue(IKMaterial* material, IKMaterialValue* value)
{
	m_Type = MATERIAL_MEMBER_TYPE_MATERIAL_VALUE;
	m_Name = value->GetName();
	m_Material = material;
	m_MaterialValue = value;
	m_PropertyView = CreatePropertyView();
	RefreshPropertyView();
}

void KEMaterialPropertyItem::InitAsBlendMode(IKMaterial* material)
{
	m_Type = MATERIAL_MEMBER_TYPE_BLEND_MODE;
	m_Name = "BlendMode";
	m_Material = material;
	m_PropertyView = CreatePropertyView();
}