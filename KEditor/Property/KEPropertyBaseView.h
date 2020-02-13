#pragma once
#include <QLayout>
#include <memory>
#include <functional>
#include <assert.h>

template<typename T, size_t DIMENSION>
class KEPropertyView;

template<typename T, size_t DIMENSION>
class KEPropertyModel;

template<typename T, size_t DIMENSION>
class KEPropertyComboView;

template<typename T, size_t DIMENSION>
class KEPropertySliderView;

template<typename T, size_t DIMENSION>
class KEPropertyCheckBoxView;

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
	template<typename T, size_t DIMENSION = 1>
	KEPropertyComboView<T, DIMENSION>* SafeComboCast()
	{
		KEPropertyComboView<T, DIMENSION>* ret = dynamic_cast<KEPropertyComboView<T, DIMENSION>*>(this);
		assert(ret);
		return ret;
	}

	template<typename T, size_t DIMENSION = 1>
	KEPropertyComboView<T, DIMENSION>* ComboCast()
	{
		return static_cast<KEPropertyComboView<T, DIMENSION>*>(this);
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

	// 转CheckBox派生类
	template<typename T, size_t DIMENSION = 1>
	KEPropertyCheckBoxView<T, DIMENSION>* SafeCheckBoxCast()
	{
		KEPropertyCheckBoxView<T, DIMENSION>* ret = dynamic_cast<KEPropertyCheckBoxView<T, DIMENSION>*>(this);
		assert(ret);
		return ret;
	}

	template<typename T, size_t DIMENSION = 1>
	KEPropertyCheckBoxView<T, DIMENSION>* CheckBoxCast()
	{
		return static_cast<KEPropertyCheckBoxView<T, DIMENSION>*>(this);
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
	std::list<std::function<void(ModelType)>> m_Listener;

	virtual void SetWidgetValue(size_t index, const T& value) = 0;

	void UpdateModel(const ModelType& value)
	{
		if (m_Model)
		{
			*m_Model = value;
			CallListener();
		}
	}

	void UpdateModelElement(size_t index, const T& value)
	{
		if (m_Model)
		{
			assert(index < DIMENSION);
			(*m_Model)[index] = value;
			CallListener();
		}
	}

	T GetModelElement(size_t index) const
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

	void CallListener()
	{
		for (const auto& func : m_Listener)
		{
			func(*m_Model);
		}
	}

	void CallSingleListener()
	{
		static_assert(DIMENSION == 1, "dimension must be 1");
		const T& value = (*m_Model)[0];
		for (const auto& func : m_SingleListener)
		{
			func(value);
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

	void AddListener(std::function<void(ModelType)> listener)
	{
		m_Listener.push_back(listener);
	}

	void RemoveAllListener()
	{
		m_Listener.clear();
	}
};
