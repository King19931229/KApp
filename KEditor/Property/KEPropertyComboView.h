#pragma once
#include "KEditorConfig.h"
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"
#include <QComboBox>
#include <string>
#include <unordered_map>
#include <assert.h>

template<typename T, size_t DIMENSION = 1>
class KEPropertyComboView : public KEPropertyView<T, DIMENSION>
{
public:
	typedef std::unordered_map<std::string, T> TextToEnumMapType;
	typedef std::unordered_map<T, std::string> EnumToTextMapType;
protected:
	QWidget* m_MainWidget;
	QComboBox *m_Widget[DIMENSION];
	QHBoxLayout *m_Layout;

	TextToEnumMapType m_TextToEnum;
	EnumToTextMapType m_EnumToText;

	void SetModelData(size_t index, const QString& text)
	{
		assert(index < DIMENSION);
		if (text.size() > 0)
		{
			std::string currentText = text.toStdString();
			auto it = m_TextToEnum.find(currentText);
			assert(it != m_TextToEnum.end());
			if (it != m_TextToEnum.end())
			{
				T newValue = it->second;
				UpdateModelElement(index, newValue);
			}
		}
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		assert(index < DIMENSION);
		auto it = m_EnumToText.find(value);
		assert(it != m_EnumToText.end());
		if (it != m_EnumToText.end())
		{
			std::string newText = it->second;
			int idx = m_Widget[index]->findText(newText.c_str());
			assert(idx >= 0);
			if (idx >= 0)
			{
				m_Widget[index]->setCurrentIndex(idx);
			}
		}
	}

	void RefreshComboItem()
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			T value = GetModelElement(i);

			m_Widget[i]->clear();
			for (const auto& pair : m_TextToEnum)
			{
				m_Widget[i]->addItem(pair.first.c_str());
			}

			auto it = m_EnumToText.find(value);
			if (it != m_EnumToText.end())
			{
				const std::string& text = it->second;
				int idx = m_Widget[i]->findText(text.c_str());
				if (idx >= 0)
				{
					m_Widget[i]->setCurrentIndex(idx);
				}
			}
		}
	}
public:
	KEPropertyComboView(ModelPtrType model)
		: KEPropertyView(model)
	{
		m_MainWidget = new QWidget();

		m_Layout = new QHBoxLayout(m_MainWidget);
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Widget[i] = new QComboBox(m_MainWidget);
			m_Widget[i]->setCurrentIndex(-1);

			QObject::connect(m_Widget[i], &QComboBox::currentTextChanged,
				[=, this](const QString& newText)
			{
				SetModelData(i, newText);
			});

			m_Layout->addWidget(m_Widget[i]);
		}

		m_MainWidget->setLayout(m_Layout);
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
		SAFE_DELETE(m_MainWidget);
	}

	QWidget* GetWidget() override
	{
		return m_MainWidget;
	}

	QWidget* MoveWidget() override
	{
		QWidget* ret = m_MainWidget;
		m_MainWidget = nullptr;
		return ret;
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