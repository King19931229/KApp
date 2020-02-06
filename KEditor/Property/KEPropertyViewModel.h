#pragma once
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"

template<typename T, size_t DIMENSION = 1>
class KEPropertyViewModel
{
protected:
	typedef KEPropertyModel<T, DIMENSION> ModelType;
	typedef KEPropertyBaseView<T, DIMENSION> BaseViewType;

	friend class BaseViewType;

	ModelType& m_Model;
	BaseViewType& m_View;

	void UpdateView()
	{
		m_View.UpdateView(m_Model);
	}

	void UpdateModel(const ModelType& model)
	{
		m_Model = model;
	}

	void UpdateModelElement(size_t index, const T& value)
	{
		m_Model[index] = value;
	}
public:
	KEPropertyViewModel(ModelType& model, BaseViewType& view)
		: m_Model(model),
		m_View(view)
	{
		m_View.SetViewModel(*this);
	}

	void SetValue(const ModelType& model)
	{
		m_Model = model;
		UpdateView();
	}

	ModelType GetValue()
	{
		return m_Model;
	}
};