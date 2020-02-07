#pragma once
#include "KEditorConfig.h"
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"
#include <QSlider>
#include <QSpinBox>
#include <string>
#include <assert.h>

template<typename T, size_t DIMENSION = 1>
class KEPropertySliderView : public KEPropertyView<T, DIMENSION>
{
protected:
	T m_Min, m_Max;
	QSlider* m_Slider[DIMENSION];
	QSpinBox* m_SpinBox[DIMENSION];
	QHBoxLayout* m_SubLayout[DIMENSION];
	QVBoxLayout *m_Layout;

	template<typename T2>
	void TypeCheck(T2)
	{
		static_assert(false, "only int is supported");
	}

	template<>
	void TypeCheck<int>(int)
	{
	}

	void SetModelData(size_t index, int newValue)
	{
		UpdateModelElement(index, (T)newValue);
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		assert(value >= m_Min);
		assert(value <= m_Max);
		assert(index < DIMENSION);

		m_SpinBox[index]->setValue((int)value);
	}

	T FindMinValue()
	{
		T ret = GetModelElement(0);
		for (size_t i = 1; i < DIMENSION; ++i)
		{
			ret = std::min(GetModelElement(i), ret);
		}
		return ret;
	}

	T FindMaxValue()
	{
		T ret = GetModelElement(0);
		for (size_t i = 1; i < DIMENSION; ++i)
		{
			ret = std::max(GetModelElement(i), ret);
		}
		return ret;
	}

	void OnRangeChange()
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Slider[i]->setMinimum((int)m_Min);
			m_Slider[i]->setMaximum((int)m_Max);

			m_SpinBox[i]->setMinimum((int)m_Min);
			m_SpinBox[i]->setMaximum((int)m_Max);
		}
	}
public:
	KEPropertySliderView(ModelPtrType model)
		: KEPropertyView(model)
	{
		TypeCheck(T());

		m_Min = FindMinValue();
		m_Max = FindMaxValue();

		m_Layout = new QVBoxLayout();

		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_SubLayout[i] = new QHBoxLayout();

			m_Slider[i] = new QSlider();
			m_SpinBox[i] = new QSpinBox();

			m_Slider[i]->setOrientation(Qt::Horizontal);
			m_Slider[i]->setSingleStep(1);

			QObject::connect(m_SpinBox[i], SIGNAL(valueChanged(int)),
				m_Slider[i], SLOT(setValue(int)));
			QObject::connect(m_Slider[i], SIGNAL(valueChanged(int)),
				m_SpinBox[i], SLOT(setValue(int)));

			QObject::connect(m_Slider[i], &QSlider::valueChanged, [=, this](int newValue)
			{
				SetModelData(i, newValue);
			});

			m_SubLayout[i]->addWidget(m_SpinBox[i]);
			m_SubLayout[i]->addWidget(m_Slider[i]);

			m_Layout->addLayout(m_SubLayout[i]);
		}

		OnRangeChange();
		UpdateView(*m_Model);
	}

	void SetRange(T min, T max)
	{
		assert(max >= min);
		assert(max >= FindMaxValue());
		assert(min <= FindMinValue());

		m_Min = min;
		m_Max = max;

		OnRangeChange();
	}

	~KEPropertySliderView()
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			SAFE_DELETE(m_SubLayout[i]);
			SAFE_DELETE(m_Slider[i]);
			SAFE_DELETE(m_SpinBox[i]);
		}
		SAFE_DELETE(m_Layout);
	}

	QLayout* GetLayout() override
	{
		return m_Layout;
	}
};

namespace KEditor
{
	template<typename T, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeSliderEditViewByModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(new KEPropertySliderView<T>(
				std::forward<Types>(args)...));
	}

	template<typename T, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeSliderEditView(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(new KEPropertySliderView<T>(
				KEditor::MakePropertyModel<T, 1>(std::forward<Types>(args)...)));
	}
}