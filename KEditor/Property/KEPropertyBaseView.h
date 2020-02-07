#pragma once
#include <QLayout>
#include <memory>

template<typename T, size_t DIMENSION>
class KEPropertyView;

template<typename T, size_t DIMENSION>
class KEPropertyModel;

template<typename T>
class KEPropertyComboView;

class KEPropertyBaseView
{
public:
	typedef std::shared_ptr<KEPropertyBaseView> BasePtr;
	virtual ~KEPropertyBaseView() {}
	virtual QLayout* GetLayout() = 0;

	//
	template<typename T, size_t DIMENSION = 1>
	KEPropertyView<T, DIMENSION>* DynamicCast()
	{
		return dynamic_cast<KEPropertyView<T, DIMENSION>*>(this);
	}

	template<typename T, size_t DIMENSION = 1>
	KEPropertyView<T, DIMENSION>* StaticCast()
	{
		return static_cast<KEPropertyView<T, DIMENSION>*>(this);
	}

	//
	template<typename T>
	KEPropertyComboView<T>* StaticComboCast()
	{
		return static_cast<KEPropertyComboView<T>*>(this);
	}

	template<typename T>
	KEPropertyComboView<T>* DynamicComboCast()
	{
		return dynamic_cast<KEPropertyComboView<T>*>(this);
	}
};

template<typename T, size_t DIMENSION = 1>
class KEPropertyView : public KEPropertyBaseView
{
public:
	typedef KEPropertyModel<T, DIMENSION> ModelType;
	typedef std::shared_ptr<ModelType> ModelPtrType;
protected:
	ModelPtrType m_Model;

	virtual void SetWidgetValue(size_t index, const T& value) = 0;

	void UpdateModel(const ModelType& value)
	{
		if (m_Model)
		{
			*m_Model = value;
		}
	}

	void UpdateModelElement(size_t index, const T& value)
	{
		if (m_Model)
		{
			(*m_Model)[index] = value;
		}
	}

	void UpdateView(const ModelType& value)
	{
		for (size_t i = 0; i < value.Diemension(); ++i)
		{
			SetWidgetValue(i, value[i]);
		}
	}
public:
	KEPropertyView(ModelPtrType model)
		: m_Model(model)
	{
	}

	void SetValue(const ModelType& model)
	{
		*m_Model = model;
		UpdateView(*m_Model);
	}

	ModelType GetValue() const
	{
		return *m_Model;
	}
};
