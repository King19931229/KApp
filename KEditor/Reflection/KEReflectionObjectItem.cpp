#include "KEReflectionObjectItem.h"

#include "Property/KEPropertyComboView.h"
#include "Property/KEPropertyLineEditView.h"
#include "Property/KEPropertySliderView.h"
#include "Property/KEPropertyCheckBoxView.h"

#include "KEditorGlobal.h"

#include <algorithm>

KEReflectionObjectItem::KEReflectionObjectItem()
	: m_Type(OBJECT_MEMBER_TYPE_SUB_NONE),
	m_Index(0),
	m_NumChildren(0),
	m_Object(nullptr),
	m_Parent(nullptr),
	m_Children(nullptr),
	m_PropertyView(nullptr)
{
}

KEReflectionObjectItem::KEReflectionObjectItem(KEReflectionObjectItem* parent, KReflectionObjectBase* object, const std::string& name)
	: m_Name(name),
	m_Type(OBJECT_MEMBER_TYPE_SUB_OBJECT),
	m_Index(0),
	m_NumChildren(0),
	m_Object(object),
	m_ParentObjects(parent ? decltype(m_ParentObjects){ parent->m_Object } : decltype(m_ParentObjects){}),
	m_Parent(parent),
	m_Children(nullptr),
	m_PropertyView(nullptr)
{
	auto type = KRTTR_GET_TYPE(object);
	ASSERT_RESULT(type.is_valid());

	m_NumChildren = type.get_properties().size();

	m_Children = KNEW KEReflectionObjectItem*[m_NumChildren];
	ZERO_ARRAY_MEMORY(m_Children);

	size_t idx = 0;
	for (auto& prop : type.get_properties())
	{
		auto prop_name = prop.get_name();
		auto prop_value = prop.get_value(object);
		auto prop_meta = prop.get_metadata(META_DATA_TYPE);

		if (prop_value.is_valid() && prop_meta.is_valid())
		{
			MetaDataType metaDataType = prop_meta.get_value<MetaDataType>();
			switch (metaDataType)
			{
			case MDT_INT:
			case MDT_FLOAT:
			case MDT_STDSTRING:
			case MDT_FLOAT2:
			case MDT_FLOAT3:
			case MDT_FLOAT4:
			{
				m_Children[idx++] = KNEW KEReflectionObjectItem(this, prop_name.to_string());
				break;
			}

			case MDT_OBJECT:
			{
				KReflectionObjectBase* subObject = prop_value.get_value<KReflectionObjectBase*>();
				if (subObject)
				{
					m_Children[idx++] = KNEW KEReflectionObjectItem(this, subObject, prop_name.to_string());
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
	}

	std::sort(m_Children, m_Children + m_NumChildren, [](const KEReflectionObjectItem* lhs, const KEReflectionObjectItem* rhs)->bool
	{
		if (lhs->m_Type != rhs->m_Type)
		{
			return lhs->m_Type < rhs->m_Type;
		}
		return lhs->m_Name < rhs->m_Name;
	});

	for (size_t i = 0; i < m_NumChildren; ++i)
	{
		m_Children[i]->m_Index = i;
	}
}

KEReflectionObjectItem::KEReflectionObjectItem(KEReflectionObjectItem* parent, const std::string& name)
	: m_Name(name),
	m_Type(OBJECT_MEMBER_TYPE_PROPERTY),
	m_Index(0),
	m_NumChildren(0),
	m_Object(nullptr),
	m_ParentObjects(parent ? decltype(m_ParentObjects){ parent->m_Object } : decltype(m_ParentObjects){}),
	m_Parent(parent),
	m_Children(nullptr),
	m_PropertyView(nullptr)
{
	m_PropertyView = CreatePropertyView();
}

KEReflectionObjectItem::~KEReflectionObjectItem()
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

KEReflectionObjectItem* KEReflectionObjectItem::GetChild(size_t childIndex)
{
	if (childIndex < m_NumChildren)
	{
		return m_Children[childIndex];
	}
	return nullptr;
}

KEReflectionObjectItem* KEReflectionObjectItem::FindChild(const std::string& name)
{
	for (size_t i = 0; i < m_NumChildren; ++i)
	{
		if (m_Children[i]->m_Name == name)
		{
			return m_Children[i];
		}
	}
	return nullptr;
}

void KEReflectionObjectItem::Merge(KReflectionObjectBase* object)
{
	auto type = KRTTR_GET_TYPE(m_Object);
	ASSERT_RESULT(type.is_valid());

	auto object_type = KRTTR_GET_TYPE(object);
	ASSERT_RESULT(object_type.is_valid());

	if (type != object_type)
	{
		return;
	}

	for (size_t i = 0; i < m_NumChildren;)
	{
		std::string propName = m_Children[i]->m_Name;
		ObjectMemberType memberType = m_Children[i]->m_Type;

		auto prop = type.get_property(propName);
		auto prop_meta = prop.get_metadata(META_DATA_TYPE);

		if (prop.is_valid() && prop_meta.is_valid())
		{
			if (memberType == OBJECT_MEMBER_TYPE_PROPERTY)
			{
				m_Children[i]->m_ParentObjects.insert(object);
			}
			else if (memberType == OBJECT_MEMBER_TYPE_SUB_OBJECT)
			{
				auto prop_value = prop.get_value(object);
				m_Children[i]->Merge(prop_value.get_value<KReflectionObjectBase*>());
			}

			++i;
		}
		else
		{
			--m_NumChildren;
			if (m_NumChildren > 0)
			{
				for (size_t j = 0; i < m_NumChildren - 1; ++j)
				{
					m_Children[j] = m_Children[j + 1];
				}
			}
		}
	}
}

void KEReflectionObjectItem::Refresh()
{
	if (m_Type == OBJECT_MEMBER_TYPE_PROPERTY)
	{
		RefreshPropertyView();
	}
	else if (m_Type == OBJECT_MEMBER_TYPE_SUB_OBJECT)
	{
		for (size_t i = 0; i < m_NumChildren; ++i)
		{
			m_Children[i]->Refresh();
		}
	}
}

void KEReflectionObjectItem::RefreshAccuraetly(KReflectionObjectBase* object)
{
	if (m_Type == OBJECT_MEMBER_TYPE_PROPERTY)
	{
		if (m_ParentObjects.find(object) != m_ParentObjects.end())
		{
			RefreshPropertyView();
		}
	}
	else if (m_Type == OBJECT_MEMBER_TYPE_SUB_OBJECT)
	{
		if (m_Object == object)
		{
			for (size_t i = 0; i < m_NumChildren; ++i)
			{
				m_Children[i]->Refresh();
			}
		}
		else
		{
			for (size_t i = 0; i < m_NumChildren; ++i)
			{
				m_Children[i]->RefreshAccuraetly(object);
			}
		}
	}
}

KEPropertyBaseView::BasePtr KEReflectionObjectItem::CreatePropertyView()
{
	KEPropertyBaseView::BasePtr ret = nullptr;

	ASSERT_RESULT(!m_ParentObjects.empty());
	KReflectionObjectBase* object = *m_ParentObjects.begin();

	auto type = KRTTR_GET_TYPE(object);
	ASSERT_RESULT(type.is_valid());

	auto prop = type.get_property(m_Name);
	ASSERT_RESULT(prop.is_valid());

	auto prop_name = prop.get_name();
	auto prop_value = prop.get_value(object);
	auto prop_meta = prop.get_metadata(META_DATA_TYPE);

	if (prop_value.is_valid() && prop_meta.is_valid())
	{
		MetaDataType metaDataType = prop_meta.get_value<MetaDataType>();
		switch (metaDataType)
		{
		case MDT_INT:
		{
			int value = prop_value.get_value<int>();

			auto intView = KEditor::MakeLineEditView<int>(value);
			intView->Cast<int>()->AddListener([this, prop_name](int value)
			{
				for (KReflectionObjectBase* object : m_ParentObjects)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);

						auto notifyData = prop_write_value.get_metadata(META_DATA_NOTIFY);
						if (notifyData.is_valid() && notifyData.get_value<MetaDataNotify>() == MDN_EDITOR)
						{
							KEditorGlobal::ReflectionManager.NotifyToEditor(object);
						}
					}
				}
			});

			ret = intView;
			break;
		}

		case MDT_FLOAT:
		{
			float value = prop_value.get_value<float>();

			auto floatView = KEditor::MakeLineEditView<float>(value);
			floatView->Cast<float>()->AddListener([this, prop_name](float value)
			{
				for (KReflectionObjectBase* object : m_ParentObjects)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);

						auto notifyData = prop_write_value.get_metadata(META_DATA_NOTIFY);
						if (notifyData.is_valid() && notifyData.get_value<MetaDataNotify>() == MDN_EDITOR)
						{
							KEditorGlobal::ReflectionManager.NotifyToEditor(object);
						}
					}
				}
			});

			ret = floatView;
			break;
		}

		case MDT_STDSTRING:
		{
			std::string value = prop_value.get_value<std::string>();

			auto stringView = KEditor::MakeLineEditView<std::string>(value);
			stringView->Cast<std::string>()->AddListener([this, prop_name](const std::string& value)
			{
				for (KReflectionObjectBase* object : m_ParentObjects)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, value);

						auto notifyData = prop_write_value.get_metadata(META_DATA_NOTIFY);
						if (notifyData.is_valid() && notifyData.get_value<MetaDataNotify>() == MDN_EDITOR)
						{
							KEditorGlobal::ReflectionManager.NotifyToEditor(object);
						}
					}
				}
			});

			ret = stringView;
			break;
		}

		case MDT_FLOAT2:
		{
			glm::vec2 value = prop_value.get_value<glm::vec2>();

			auto vecView = KEditor::MakeLineEditView<float, 2>({ value[0], value[1] });
			vecView->Cast<float, 2>()->AddListener([this, prop_name](const auto& float2value)
			{
				for (KReflectionObjectBase* object : m_ParentObjects)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, glm::vec2(float2value[0], float2value[1]));

						auto notifyData = prop_write_value.get_metadata(META_DATA_NOTIFY);
						if (notifyData.is_valid() && notifyData.get_value<MetaDataNotify>() == MDN_EDITOR)
						{
							KEditorGlobal::ReflectionManager.NotifyToEditor(object);
						}
					}
				}
			});

			ret = vecView;
			break;
		}

		case MDT_FLOAT3:
		{
			glm::vec3 value = prop_value.get_value<glm::vec3>();

			auto vecView = KEditor::MakeLineEditView<float, 3>({ value[0], value[1], value[2] });
			vecView->Cast<float, 3>()->AddListener([this, prop_name](const auto& float3value)
			{
				for (KReflectionObjectBase* object : m_ParentObjects)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, glm::vec3(float3value[0], float3value[1], float3value[2]));

						auto notifyData = prop_write_value.get_metadata(META_DATA_NOTIFY);
						if (notifyData.is_valid() && notifyData.get_value<MetaDataNotify>() == MDN_EDITOR)
						{
							KEditorGlobal::ReflectionManager.NotifyToEditor(object);
						}
					}
				}
			});

			ret = vecView;
			break;
		}

		case MDT_FLOAT4:
		{
			glm::vec4 value = prop_value.get_value<glm::vec4>();

			auto vecView = KEditor::MakeLineEditView<float, 4>({ value[0], value[1], value[2], value[3] });
			vecView->Cast<float, 4>()->AddListener([this, prop_name](const auto& float4value)
			{
				for (KReflectionObjectBase* object : m_ParentObjects)
				{
					auto type = KRTTR_GET_TYPE(object);
					auto prop_write_value = type.get_property(prop_name);
					if (!prop_write_value.is_readonly())
					{
						prop_write_value.set_value(*object, glm::vec4(float4value[0], float4value[1], float4value[2], float4value[3]));

						auto notifyData = prop_write_value.get_metadata(META_DATA_NOTIFY);
						if (notifyData.is_valid() && notifyData.get_value<MetaDataNotify>() == MDN_EDITOR)
						{
							KEditorGlobal::ReflectionManager.NotifyToEditor(object);
						}
					}
				}
			});

			ret = vecView;
			break;
		}

		default:
			assert(false && "should not reach");
			break;
		}
	}

	return ret;
}

void KEReflectionObjectItem::RefreshPropertyView()
{
	ASSERT_RESULT(m_PropertyView);

	ASSERT_RESULT(!m_ParentObjects.empty());
	KReflectionObjectBase* object = *m_ParentObjects.begin();

	auto type = KRTTR_GET_TYPE(object);
	ASSERT_RESULT(type.is_valid());

	auto prop = type.get_property(m_Name);
	ASSERT_RESULT(prop.is_valid());

	auto prop_value = prop.get_value(object);
	auto prop_meta = prop.get_metadata(META_DATA_TYPE);

	if (prop_value.is_valid() && prop_meta.is_valid())
	{
		MetaDataType metaDataType = prop_meta.get_value<MetaDataType>();
		switch (metaDataType)
		{
		case MDT_INT:
		{
			int value = prop_value.get_value<int>();
			// 这里考虑的原因比较复杂 总之就是不能够再调用listener 否则对多选属性面板与效率上都是问题
			auto guard = m_PropertyView->CreateListenerMuteGuard();
			m_PropertyView->Cast<int>()->SetValue(value);
			break;
		}

		case MDT_FLOAT:
		{
			float value = prop_value.get_value<float>();
			auto guard = m_PropertyView->CreateListenerMuteGuard();
			m_PropertyView->Cast<float>()->SetValue(value);
			break;
		}

		case MDT_STDSTRING:
		{
			std::string value = prop_value.get_value<std::string>();
			auto guard = m_PropertyView->CreateListenerMuteGuard();
			m_PropertyView->Cast<std::string>()->SetValue(value);
			break;
		}

		case MDT_FLOAT2:
		{
			glm::vec2 value = prop_value.get_value<glm::vec2>();
			auto guard = m_PropertyView->CreateListenerMuteGuard();
			m_PropertyView->Cast<float, 2>()->SetValue({ value[0], value[1] });
			break;
		}

		case MDT_FLOAT3:
		{
			glm::vec3 value = prop_value.get_value<glm::vec3>();
			auto guard = m_PropertyView->CreateListenerMuteGuard();
			m_PropertyView->Cast<float, 3>()->SetValue({ value[0], value[1], value[2] });
			break;
		}

		case MDT_FLOAT4:
		{
			glm::vec4 value = prop_value.get_value<glm::vec4>();
			auto guard = m_PropertyView->CreateListenerMuteGuard();
			m_PropertyView->Cast<float, 4>()->SetValue({ value[0], value[1], value[2], value[3] });
			break;
		}

		default:
			assert(false && "should not reach");
			break;
		}
	}
}