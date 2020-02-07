#pragma once
#include "KEditorConfig.h"
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"
#include <QComboBox.h>
#include <string>
#include <unordered_map>
#include <assert.h>

template<typename T>
class KEPropertyComboView : public KEPropertyView<T, 1>
{
public:
	typedef std::unordered_map<std::string, T> TextToEnumMapType;
	typedef std::unordered_map<T, std::string> EnumToTextMapType;
protected:
	QComboBox *m_Widget;
	QHBoxLayout *m_Layout;

	TextToEnumMapType m_TextToEnum;
	EnumToTextMapType m_EnumToText;

	void SetModelData(const QString& text)
	{
		if (text.size() > 0)
		{
			std::string currentText = text.toStdString();
			auto it = m_TextToEnum.find(currentText);
			assert(it != m_TextToEnum.end());
			if (it != m_TextToEnum.end())
			{
				T newValue = it->second;
				UpdateModelElement(0, newValue);
			}
		}
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		auto it = m_EnumToText.find(value);
		assert(it != m_EnumToText.end());
		if (it != m_EnumToText.end())
		{
			std::string newText = it->second;
			int idx = m_Widget->findText(newText.c_str());
			assert(idx >= 0);
			if (idx >= 0)
			{
				m_Widget->setCurrentIndex(idx);
			}
		}
	}

	void RefreshComboItem()
	{
		std::string currentText = m_Widget->currentText().toStdString();

		m_Widget->clear();
		for (const auto& pair : m_TextToEnum)
		{
			m_Widget->addItem(pair.first.c_str());
		}

		if (m_TextToEnum.find(currentText) != m_TextToEnum.end())
		{
			int idx = m_Widget->findText(currentText.c_str());
			if (idx >= 0)
			{
				m_Widget->setCurrentIndex(idx);
			}
		}
	}
public:
	KEPropertyComboView(ModelPtrType model)
		: KEPropertyView(model)
	{
		m_Layout = new QHBoxLayout();
		m_Widget = new QComboBox();
		m_Layout->addWidget(m_Widget);

		QObject::connect(m_Widget, &QComboBox::currentTextChanged,
			[=, this](const QString& newText)
		{
			SetModelData(newText);
		});

		// UpdateView(*m_Model);
	}

	void AppendMapping(T enumeration, const std::string& text)
	{
		{
			auto it = m_EnumToText.find(enumeration);
			if (it != m_EnumToText.end())
			{
				std::string oldText = it->second;
				auto it2 = m_TextToEnum.find(oldText);
				if (it2 != m_TextToEnum.end())
				{
					m_TextToEnum.erase(it2);
				}
				m_EnumToText.erase(it);
			}
		}
		m_EnumToText[enumeration] = text;
		m_TextToEnum[text] = enumeration;
		RefreshComboItem();
	}

	void SetMapping(const EnumToTextMapType& enum2Text)
	{
		m_EnumToText = enum2Text;

		m_TextToEnum.clear();
		for (const auto& pair : m_EnumToText)
		{
			m_TextToEnum[pair.second] = pair.first;
		}

		RefreshComboItem();
	}

	~KEPropertyComboView()
	{
		SAFE_DELETE(m_Layout);
		SAFE_DELETE(m_Widget);
	}

	QLayout* GetLayout() override
	{
		return m_Layout;
	}
};

namespace KEditor
{
	template<typename T, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeComboEditViewByModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(new KEPropertyComboView<T>(
				std::forward<Types>(args)...));
	}

	template<typename T, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeComboEditView(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(new KEPropertyComboView<T>(
				KEditor::MakePropertyModel<T, 1>(std::forward<Types>(args)...)));
	}
}