#pragma once
#include <QLayout>
#include <memory>
#include <assert.h>

template<typename T, size_t DIMENSION>
class KEPropertyView;

template<typename T, size_t DIMENSION>
class KEPropertyModel;

template<typename T>
class KEPropertyComboView;

template<typename T, size_t DIMENSION>
class KEPropertySliderView;

class KEPropertyBaseView
{
public:
	typedef std::shared_ptr<KEPropertyBaseView> BasePtr;
	virtual ~KEPropertyBaseView() {}
	virtual QLayout* GetLayout() = 0;

	// 转基础派生类
	template<typename T, size_t DIMENSION = 1>
	KEPropertyView<T, DIMENSION>* SafeCast()
	{
		KEPropertyView<T, DIMENSION>* ret = dynamic_cast<KEPropertyView<T, DIMENSION>*>(this);
		assert(ret);
		return ret;
	}

	template<typename T, size_t DIMENSION = 1>
	KEPropertyView<T, DIMENSION>* Cast()
	{
		return static_cast<KEPropertyView<T, DIMENSION>*>(this);
	}

	// 转Combo派生类
	template<typename T>
	KEPropertyComboView<T>* SafeComboCast()
	{
		KEPropertyComboView<T>* ret = dynamic_cast<KEPropertyComboView<T>*>(this);
		assert(ret);
		return ret;
	}

	template<typename T>
	KEPropertyComboView<T>* ComboCast()
	{
		return static_cast<KEPropertyComboView<T>*>(this);
	}

	// 转Slider派生类
	template<typename T, size_t DIMENSION = 1>
	KEPropertySliderView<T, DIMENSION>* SafeSliderCast()
	{
		KEPropertySliderView<T, DIMENSION>* ret = dynamic_cast<KEPropertySliderView<T, DIMENSION>*>(this);
		assert(ret);
		return ret;
	}

	template<typename T, size_t DIMENSION = 1>
	KEPropertySliderView<T, DIMENSION>* SliderCast()
	{
		return static_cast<KEPropertySliderView<T, DIMENSION>*>(this);
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
			assert(index < DIMENSION);
			(*m_Model)[index] = value;
		}
	}

	T GetModelElement(size_t index)
	{
		if (m_Model)
		{
			assert(index < DIMENSION);
			return (*m_Model)[index];
		}
		return T();
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
