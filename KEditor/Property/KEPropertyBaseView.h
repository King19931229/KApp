#pragma once
#include <QLayout>
template<typename T, size_t DIMENSION>
class KEPropertyViewModel;

class KEPropertyWidget
{
public:
	virtual QLayout* GetLayout() = 0;
};

template<typename T, size_t DIMENSION>
class KEPropertyBaseView : public KEPropertyWidget
{
protected:
	typedef KEPropertyModel<T, DIMENSION> ModelType;
	typedef KEPropertyViewModel<T, DIMENSION> ViewType;
	ViewType* m_ViewModel;

	virtual void SetWidgetValue(size_t index, const T& value) = 0;
public:
	KEPropertyBaseView()
		: m_ViewModel(nullptr)
	{
	}

	void SetViewModel(ViewType& viewModel)
	{
		m_ViewModel = &viewModel;
	}

	void UpdateModel(const ModelType& value)
	{
		if (m_ViewModel)
		{
			m_ViewModel->UpdateModel(value);
		}
	}

	void UpdateModelElement(size_t index, const T& value)
	{
		if (m_ViewModel)
		{
			m_ViewModel->UpdateModelElement(index, value);
		}
	}

	void UpdateView(const ModelType& value)
	{
		for (size_t i = 0; i < value.Diemension(); ++i)
		{
			SetWidgetValue(i, value[i]);
		}
	}
};
