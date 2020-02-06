#pragma once
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"

template<typename T, size_t DIMENSION = 1>
class KEPropertyViewModel
{
public:
	typedef std::shared_ptr<KEPropertyViewModel<T, DIMENSION>> PtrType;
protected:
	typedef KEPropertyModel<T, DIMENSION> ModelType;
	typedef KEPropertyView<T, DIMENSION> BaseViewType;

	typedef std::shared_ptr<ModelType> ModelTypePtr;
	typedef std::shared_ptr<BaseViewType> BaseViewTypePtr;

	friend class BaseViewType;

	ModelTypePtr m_Model;
	BaseViewTypePtr m_View;

	void UpdateView()
	{
		m_View.UpdateView(m_Model.get());
	}

	void UpdateModel(const ModelType& model)
	{
		*m_Model = model;
	}

	void UpdateModelElement(size_t index, const T& value)
	{
		(*m_Model)[index] = value;
	}
public:
	KEPropertyViewModel(ModelTypePtr model, BaseViewTypePtr view)
		: m_Model(model),
		m_View(view)
	{
		m_View->SetViewModel(*this);
	}

	void SetValue(const ModelType& model)
	{
		*m_Model = model;
		UpdateView();
	}

	ModelType GetValue() const
	{
		return *m_Model;
	}

	inline ModelTypePtr GetModel() { return m_Model; }
	inline BaseViewTypePtr GetView() { return m_View; }
};

namespace KEditor
{
	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::unique_ptr<KEPropertyViewModel<T, DIMENTSION>> MakePropertyViewModelPtr(Types&&... args)
	{
		return std::unique_ptr<KEPropertyViewModel<T, DIMENTSION>>
			(new KEPropertyViewModel<T, DIMENTSION>(std::forward<Types>(args)...));
	}
}