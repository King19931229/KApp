#pragma once
#include "KEditorConfig.h"
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"
#include <QCheckBox>
#include <assert.h>

template<typename T, size_t DIMENSION = 1>
class KEPropertyCheckBoxView : public KEPropertyView<T, DIMENSION>
{
protected:
	QWidget* m_MainWidget;
	QCheckBox* m_Widget[DIMENSION];
	QHBoxLayout *m_Layout;

	template<typename T2>
	void TypeCheck(T2)
	{
		static_assert(false, "only bool is supported");
	}

	template<>
	void TypeCheck<bool>(bool)
	{
	}

	void SetModelData(size_t index, int newValue)
	{
		assert(newValue != Qt::PartiallyChecked);
		bool bValue = (newValue == Qt::Unchecked) ? false : true;
		UpdateModelElement(index, (T)bValue);
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		bool bCheck = (bool)value;
		assert(index < DIMENSION);
		if (m_Widget[index])
		{
			m_Widget[index]->setChecked(bCheck);
		}
	}
public:
	KEPropertyCheckBoxView(ModelPtrType model)
		: KEPropertyView(model)
		m_MainWidget(nullptr),
		m_Layout(nullptr)
	{
		ZERO_ARRAY_MEMROY(m_Widget);
		TypeCheck(T());
	}

	~KEPropertyCheckBoxView()
	{
	}

	QWidget* AllocWidget() override
	{
		m_MainWidget = KNEW QWidget();
		m_Layout = KNEW QHBoxLayout();

		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Widget[i] = KNEW QCheckBox();
			m_Widget[i]->setTristate(false);

			QObject::connect(m_Widget[i], &QCheckBox::stateChanged, [=, this](int newValue)
			{
				SetModelData(i, newValue);
			});
			m_Layout->addWidget(m_Widget[i]);
		}

		m_MainWidget->setLayout(m_Layout);

		UpdateView(*m_Model);

		return m_MainWidget;
	}
};

namespace KEditor
{
	template<typename T, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeCheckBoxViewByModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyCheckBoxView<T>(
				std::forward<Types>(args)...));
	}

	template<typename T, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeCheckBoxView(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyCheckBoxView<T>(
				KEditor::MakePropertyModel<T, 1>(std::forward<Types>(args)...)));
	}
}