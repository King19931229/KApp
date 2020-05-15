#include "KEReflectionObjectItem.h"

#include "Property/KEPropertyComboView.h"
#include "Property/KEPropertyLineEditView.h"
#include "Property/KEPropertySliderView.h"
#include "Property/KEPropertyCheckBoxView.h"

#include <algorithm>

KEReflectionObjectItem::KEReflectionObjectItem()
	: m_Parent(nullptr),
	m_Object(nullptr),
	m_PropertyView(nullptr),
	m_Type(OBJECT_MEMBER_TYPE_SUB_OBJECT),
	m_Children(nullptr),
	m_NumChildren(0),
	m_Index(-1)
{
}

KEReflectionObjectItem::KEReflectionObjectItem(KEReflectionObjectItem* parent, KReflectionObjectBase* object)
	: m_Parent(parent),
	m_Object(object),
	m_PropertyView(nullptr),
	m_Type(OBJECT_MEMBER_TYPE_SUB_OBJECT),
	m_Children(nullptr),
	m_NumChildren(0)
{
	auto type = KRTTR_GET_TYPE(object);
	ASSERT_RESULT(type.is_valid());

	auto name = type.get_name();
	m_Name = name.to_string();

	m_NumChildren = type.get_properties().size();
	m_Children = new KEReflectionObjectItem[m_NumChildren];

	size_t idx = 0;
	for (auto& prop : type.get_properties())
	{
		auto prop_name = prop.get_name();
		auto prop_value = prop.get_value(object);
		auto prop_meta = prop.get_metadata(META_DATA_TYPE);

		if (prop_value.is_valid() && prop_meta.is_valid())
		{
			MetaDataType metaDataType = prop_meta.get_value<MetaDataType>();
			switch (metaDataType) {

			case MDT_INT:
			{
				int value = prop_value.get_value<int>();

				auto intView = KEditor::MakeLineEditView<int>(value);
				intView->Cast<int>()->AddListener([object, prop_name](int value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), intView);
				break;
			}

			case MDT_FLOAT:
			{
				int value = prop_value.get_value<float>();

				auto floatView = KEditor::MakeLineEditView<float>(value);
				floatView->Cast<float>()->AddListener([object, prop_name](float value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), floatView);
				break;
			}

			case MDT_CSTR:
			{
				const char* value = prop_value.get_value<const char*>();

				auto cstrView = KEditor::MakeLineEditView<std::string>(value);
				cstrView->Cast<std::string>()->AddListener([object, prop_name](const std::string& value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), cstrView);
				break;
			}

			case MDT_STDSTRING:
			{
				std::string value = prop_value.get_value<std::string>();

				auto stringView = KEditor::MakeLineEditView<std::string>(value);
				stringView->Cast<std::string>()->AddListener([object, prop_name](const std::string& value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), stringView);
				break;
			}

			case MDT_FLOAT2:
			{
				glm::vec2 value = prop_value.get_value<glm::vec2>();

				auto vecView = KEditor::MakeLineEditView<float, 2>({ value[0], value[1] });
				vecView->Cast<float, 2>()->AddListener([object, prop_name](const auto& float2value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, glm::vec2(float2value[0], float2value[1]));
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), vecView);
				break;
			}

			case MDT_FLOAT3:
			{
				glm::vec3 value = prop_value.get_value<glm::vec3>();

				auto vecView = KEditor::MakeLineEditView<float, 3>({ value[0], value[1], value[2] });
				vecView->Cast<float, 3>()->AddListener([object, prop_name](const auto& float3value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, glm::vec3(float3value[0], float3value[1], float3value[2]));
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), vecView);
				break;
			}

			case MDT_FLOAT4:
			{
				glm::vec4 value = prop_value.get_value<glm::vec4>();

				auto vecView = KEditor::MakeLineEditView<float, 4>({ value[0], value[1], value[2], value[3] });
				vecView->Cast<float, 4>()->AddListener([object, prop_name](const auto& float4value)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, glm::vec4(float4value[0], float4value[1], float4value[2], float4value[3]));
					}
				});

				m_Children[idx] = KEReflectionObjectItem(this, prop_name.to_string(), vecView);
				break;
			}

			case MDT_OBJECT:
			{
				KReflectionObjectBase* subObject = prop_value.get_value<KReflectionObjectBase*>();
				if (subObject)
				{
					m_Children[idx] = KEReflectionObjectItem(this, subObject);
				}
				else
				{
					--m_NumChildren;
				}
				break;
			}

			default:
				assert(false && "should not reach");
				break;
			}
		}
		++idx;
	}

	std::sort(m_Children, m_Children + m_NumChildren, [](const KEReflectionObjectItem&lhs, const KEReflectionObjectItem& rhs)->bool
	{
		if (lhs.m_Type != rhs.m_Type)
		{
			return lhs.m_Type < rhs.m_Type;
		}
		return lhs.m_Name < rhs.m_Name;
	});

	for (size_t i = 0; i < m_NumChildren; ++i)
	{
		m_Children[i].m_Index = i;
	}
}

KEReflectionObjectItem::KEReflectionObjectItem(KEReflectionObjectItem* parent, const std::string& name, KEPropertyBaseView::BasePtr view)
	: m_Parent(parent),
	m_Name(name),
	m_Object(nullptr),
	m_PropertyView(view),
	m_Type(OBJECT_MEMBER_TYPE_SUB_OBJECT),
	m_Children(nullptr),
	m_NumChildren(0)
{
}

KEReflectionObjectItem::~KEReflectionObjectItem()
{
	SAFE_DELETE_ARRAY(m_Children);
}

KEReflectionObjectItem* KEReflectionObjectItem::GetChild(size_t childIndex)
{
	if (childIndex < m_NumChildren)
	{
		return &m_Children[childIndex];
	}
	return nullptr;
}