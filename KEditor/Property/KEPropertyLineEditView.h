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
	QWidget* m_MainWidget;
	QLineEdit *m_Widget[DIMENSION];
	QHBoxLayout *m_Layout;

	template<typename T2>
	void SetWidgetValidator(QLineEdit& widget, const T2& default)
	{

	}

	template<>
	void SetWidgetValidator(QLineEdit& widget, const float& default)
	{
		QDoubleValidator* validator = KNEW QDoubleValidator(&widget);
		widget.setValidator(validator);
	}

	template<>
	void SetWidgetValidator(QLineEdit& widget, const int& default)
	{
		QIntValidator* validator = KNEW QIntValidator(&widget);
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
	T2 DataFromQString(const QString& text)
	{
		static_assert(false, "please implement data form qstring");
		return default;
	}

	template<>
	std::string DataFromQString(const QString& text)
	{
		return text.toStdString();
	}

	template<>
	float DataFromQString(const QString& text)
	{
		return text.toFloat();
	}

	template<>
	int DataFromQString(const QString& text)
	{
		return text.toInt();
	}

	void SetModelData(size_t index, const QString& text)
	{
		T data = DataFromQString<T>(text);
		UpdateModelElement(index, data);
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		assert(index < DIMENSION);
		if (m_Widget[index])
		{
			SetWidgetText(*m_Widget[index], value);
		}
	}
public:
	KEPropertyLineEditView(ModelPtrType model)
		: KEPropertyView(model),
		m_MainWidget(nullptr),
		m_Layout(nullptr)
	{
		ZERO_ARRAY_MEMORY(m_Widget);
	}

	~KEPropertyLineEditView()
	{
	}

	QWidget* AllocWidget() override
	{
		m_MainWidget = KNEW QWidget();

		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Widget[i] = KNEW QLineEdit(m_MainWidget);
			SetWidgetValidator(*m_Widget[i], T());
			QObject::connect(m_Widget[i], &QLineEdit::textChanged, [=, this](const QString& newText)
			{
				SetModelData(i, newText);
			});
		}

		m_Layout = KNEW QHBoxLayout(m_MainWidget);
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Layout->addWidget(m_Widget[i]);
		}

		m_MainWidget->setLayout(m_Layout);

		UpdateView(*m_Model);

		return m_MainWidget;
	}
};

namespace KEditor
{
	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditViewByModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyLineEditView<T, DIMENTSION>(
				std::forward<Types>(args)...));
	}

	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditView(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyLineEditView<T, DIMENTSION>(
				KEditor::MakePropertyModel<T, DIMENTSION>(std::forward<Types>(args)...)));
	}

	template<typename T, size_t DIMENTSION>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditView(std::initializer_list<T>&& list)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyLineEditView<T, DIMENTSION>(KEditor::MakePropertyModel<T, DIMENTSION>(
				std::forward<decltype(list)>(list))));
	}
}