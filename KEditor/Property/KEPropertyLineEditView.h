#pragma once
#include "KEditorConfig.h"
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"
#include <QLineEdit>
#include <QValidator>
#include <assert.h>

template<typename T, size_t DIMENSION = 1>
class KEPropertyLineEditView : public KEPropertyView<T, DIMENSION>
{
protected:
	QLineEdit *m_Widget[DIMENSION];
	QHBoxLayout *m_Layout;

	template<typename T2>
	void SetWidgetValidator(QLineEdit& widget, const T2& default)
	{

	}

	template<>
	void SetWidgetValidator(QLineEdit& widget, const float& default)
	{
		QDoubleValidator* validator = new QDoubleValidator(&widget);
		widget.setValidator(validator);
	}

	template<>
	void SetWidgetValidator(QLineEdit& widget, const int& default)
	{
		QIntValidator* validator = new QIntValidator(&widget);
		widget.setValidator(validator);
	}

	template<typename T2>
	void SetWidgetText(QLineEdit& widget, const T2& value)
	{
		static_assert(false, "please implement data form qstring");
	}

	template<>
	void SetWidgetText(QLineEdit& widget, const std::string& value)
	{
		widget.setText(value.c_str());
	}

	template<>
	void SetWidgetText(QLineEdit& widget, const float& value)
	{
		widget.setText(QString::number(value));
	}

	template<>
	void SetWidgetText(QLineEdit& widget, const int& value)
	{
		widget.setText(QString::number(value));
	}

	template<typename T2>
	T2 DataFromQString(const QString& text, const T2& default)
	{
		static_assert(false, "please implement data form qstring");
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
		T data = DataFromQString(text, (*m_Model)[index]);
		UpdateModelElement(index, data);
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		assert(index < DIMENSION);
		SetWidgetText(*m_Widget[index], value);
	}
public:
	KEPropertyLineEditView(ModelPtrType model)
		: KEPropertyView(model)
	{
		m_Layout = new QHBoxLayout();
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Widget[i] = new QLineEdit();
			SetWidgetValidator(*m_Widget[i], T());
			m_Layout->addWidget(m_Widget[i]);
			QObject::connect(m_Widget[i], &QLineEdit::textChanged, [=, this](const QString& newText)
			{
				SetModelData(i, newText);
			});
		}

		UpdateView(*m_Model);
	}

	~KEPropertyLineEditView()
	{
		SAFE_DELETE(m_Layout);
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			SAFE_DELETE(m_Widget[i]);
		}
	}

	QLayout* GetLayout() override
	{
		return m_Layout;
	}
};

namespace KEditor
{
	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditViewByModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(new KEPropertyLineEditView<T, DIMENTSION>(
				std::forward<Types>(args)...));
	}

	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditView(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(new KEPropertyLineEditView<T, DIMENTSION>(
				KEditor::MakePropertyModel<T, DIMENTSION>(std::forward<Types>(args)...)));
	}
}