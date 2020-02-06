#pragma once
#include <QLayout>
template<typename T, size_t DIMENSION>
class KEPropertyViewModel;

template<typename T, size_t DIMENSION>
class KEPropertyModel;

class KEPropertyBaseView
{
public:
	virtual ~KEPropertyBaseView() {}
	virtual QLayout* GetLayout() = 0;
};

template<typename T, size_t DIMENSION = 1>
class KEPropertyView : public KEPropertyBaseView
{
protected:
	typedef KEPropertyModel<T, DIMENSION> ModelType;
	typedef KEPropertyViewModel<T, DIMENSION> ViewType;

	friend class ViewType;
	ViewType* m_ViewModel;

	virtual void SetWidgetValue(size_t index, const T& value) = 0;

	void SetViewModel(ViewType& viewModel)
	{
		m_ViewModel = &viewModel;
		UpdateView(viewModel.GetValue());
	}
public:
	KEPropertyView()
		: m_ViewModel(nullptr)
	{
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
