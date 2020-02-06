#pragma once
#include "KEPropertyBaseView.h"
#include <QLineEdit>
#include <assert.h>

template<typename T, size_t DIMENSION = 1>
class KEPropertyRegularView : public KEPropertyBaseView<T, DIMENSION>
{
protected:
	QLineEdit m_Widget[DIMENSION];
	QHBoxLayout m_Layout;

	template<typename T2>
	void SetWidgetText(QLineEdit& widget, const T2& value)
	{
		widget.setText(QString::number(value));
	}

	template<>
	void SetWidgetText<std::string>(QLineEdit& widget, const std::string& value)
	{
		widget.setText(value.c_str());
	}

	template<typename T2>
	T2 DataFromQString(const QString& text, const T2& default)
	{
		static_assert(false, "Please implement it");
		return default;
	}

	template<>
	std::string DataFromQString(const QString& text, const std::string& default)
	{
		return text.toStdString();
	}

	template<>
	float DataFromQString(const QString& text, const float& default)
	{
		return text.toFloat();
	}

	template<>
	int DataFromQString(const QString& text, const int& default)
	{
		return text.toInt();
	}

	void SetModelData(size_t index, const QString& text)
	{
		T data = DataFromQString(text, T());
		UpdateModelElement(index, data);
	}
public:
	KEPropertyRegularView()
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Layout.addWidget(&m_Widget[i]);
			QObject::connect(&m_Widget[i], &QLineEdit::textChanged, [=, this](const QString& newText)
			{
				SetModelData(i, newText);
			});
		}
	}

	QLayout* GetLayout() override
	{
		return &m_Layout;
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		assert(index < DIMENSION);
		SetWidgetText(m_Widget[index], value);
	}
};