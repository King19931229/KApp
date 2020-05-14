#include "KEReflectionManager.h"

#include "Property/KEPropertyComboView.h"
#include "Property/KEPropertyLineEditView.h"
#include "Property/KEPropertySliderView.h"
#include "Property/KEPropertyCheckBoxView.h"

#include <assert.h>

KEReflectObjectWidget::KEReflectObjectWidget(const std::string& name)
{
	m_Layout = new QVBoxLayout();
}

KEReflectObjectWidget::~KEReflectObjectWidget()
{
	for (auto it = m_Properties.begin(), itEnd = m_Properties.end();
		it != itEnd; ++it)
	{
		m_Layout->removeItem(it->layout);

		it->layout->removeWidget(it->label);
		it->layout->removeItem(it->propertyView->GetLayout());

		SAFE_DELETE(it->layout);
		SAFE_DELETE(it->label);
	}
	m_Properties.clear();

	for (auto it = m_Objects.begin(), itEnd = m_Objects.end();
		it != itEnd; ++it)
	{
		m_Layout->removeWidget(it->widget);
		SAFE_DELETE(it->widget);
	}
	m_Objects.clear();

	SAFE_DELETE(m_Layout);
}

void KEReflectObjectWidget::AddProperty(const std::string& name, KEPropertyBaseView::BasePtr propertyView)
{
	auto it = std::find_if(m_Properties.begin(), m_Properties.end(), [&name](const PropertyItem& item)->bool
	{
		return item.name == name;
	});

	if (it == m_Properties.end())
	{
		PropertyItem newItem;

		newItem.layout = new QHBoxLayout();
		newItem.label = new QLabel(name.c_str());
		newItem.name = name;
		newItem.propertyView = propertyView;

		static_cast<QHBoxLayout*>(newItem.layout)->addWidget(newItem.label);
		static_cast<QHBoxLayout*>(newItem.layout)->addLayout(propertyView->GetLayout());

		m_Layout->addLayout(newItem.layout);

		m_Properties.push_back(std::move(newItem));
	}
	else
	{
		ASSERT_RESULT(false);
	}
}

void KEReflectObjectWidget::AddObject(const std::string& name, KEReflectObjectWidget* widget)
{
	auto it = std::find_if(m_Objects.begin(), m_Objects.end(), [&name](const ObjectItem& item)->bool
	{
		return item.name == name;
	});

	if (it == m_Objects.end())
	{
		ObjectItem newItem;

		newItem.name = name;
		newItem.widget = widget;

		m_Layout->addWidget(newItem.widget);

		m_Objects.push_back(std::move(newItem));
	}
	else
	{
		ASSERT_RESULT(false);
	}
}

KEReflectionManager::KEReflectionManager()
{
}

KEReflectionManager::~KEReflectionManager()
{
	assert(m_WidgetMap.empty());
}

KEReflectObjectWidget* KEReflectionManager::Build(KReflectionObjectBase* object)
{
	if (!object)
	{
		return nullptr;
	}

	auto type = KRTTR_GET_TYPE(object);
	if (!type.is_valid())
	{
		return nullptr;
	}

	auto object_name = type.get_name();

	KEReflectObjectWidget* widget = new KEReflectObjectWidget(object_name.to_string());

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

				widget->AddProperty(prop_name.to_string(), intView);
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

				widget->AddProperty(prop_name.to_string(), floatView);
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

				widget->AddProperty(prop_name.to_string(), cstrView);
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

				widget->AddProperty(prop_name.to_string(), stringView);
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

				widget->AddProperty(prop_name.to_string(), vecView);
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

				widget->AddProperty(prop_name.to_string(), vecView);
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

				widget->AddProperty(prop_name.to_string(), vecView);
				break;
			}

			case MDT_OBJECT:
			{
				KReflectionObjectBase* subObject = prop_value.get_value<KReflectionObjectBase*>();
				KEReflectObjectWidget* subWidget = Build(subObject);
				if (subWidget)
				{
					widget->AddObject(prop_name.to_string(), subWidget);
				}
				break;
			}

			default:
				break;

			}
		}
	}

	return widget;
}

bool KEReflectionManager::Init()
{
	return true;
}

bool KEReflectionManager::UnInit()
{
	for (auto& pair : m_WidgetMap)
	{
		KEReflectObjectWidget* widget = pair.second;
		SAFE_DELETE(widget);
	}
	m_WidgetMap.clear();
	return true;
}

bool KEReflectionManager::Watch(KReflectionObjectBase* object)
{
	auto it = m_WidgetMap.find(object);
	if (it == m_WidgetMap.end())
	{
		KEReflectObjectWidget* widget = Build(object);
		if (widget)
		{
			m_WidgetMap.insert({object, widget});
			return true;
		}
		return false;
	}
	return true;
}

bool KEReflectionManager::Discard(KReflectionObjectBase* object)
{
	auto it = m_WidgetMap.find(object);
	if (it != m_WidgetMap.end())
	{
		KEReflectObjectWidget* widget = it->second;
		SAFE_DELETE(widget);
		m_WidgetMap.erase(it);
	}
	return true;
}

bool KEReflectionManager::Refresh(KReflectionObjectBase* obect)
{
	return true;
}