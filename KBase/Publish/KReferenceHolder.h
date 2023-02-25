#pragma once

#include "KBase/Publish/KConfig.h"
#include <functional>

template<typename Type>
class KReference
{
public:
	typedef std::function<void(Type)> ReleaseFunction;
protected:
	Type m_Soul;
	ReleaseFunction m_ReleaseFunc;
public:
	KReference()
		: m_Soul(nullptr)
	{
	}
	KReference(Type soul, ReleaseFunction releaseFunc)
		: m_Soul(soul)
		, m_ReleaseFunc(releaseFunc)
	{
		ASSERT_RESULT(m_Soul);
	}
	~KReference()
	{
	}
	KReference(const KReference& rhs)
	{
		m_Soul = rhs.m_Soul;
		m_ReleaseFunc = rhs.m_ReleaseFunc;
	}
	KReference(KReference&& rhs)
	{
		m_Soul = std::move(rhs.m_Soul);
		m_ReleaseFunc = std::move(rhs.m_ReleaseFunc);
		rhs.m_Soul = nullptr;
		rhs.m_ReleaseFunc = ReleaseFunction();
	}
	KReference& operator=(const KReference& rhs)
	{
		m_Soul = rhs.m_Soul;
		m_ReleaseFunc = rhs.m_ReleaseFunc;
		return *this;
	}
	KReference& operator=(KReference&& rhs)
	{
		m_Soul = std::move(rhs.m_Soul);
		m_ReleaseFunc = std::move(rhs.m_ReleaseFunc);
		rhs.m_Soul = nullptr;
		rhs.m_ReleaseFunc = ReleaseFunction();
		return *this;
	}
	const Type& Get() const
	{
		return m_Soul;
	}
	const Type& operator*() const
	{
		return m_Soul;
	}
	Type& Get()
	{
		return m_Soul;
	}
	Type& operator*()
	{
		return m_Soul;
	}
	void Release()
	{
		m_ReleaseFunc(m_Soul);
		m_Soul = nullptr;
		m_ReleaseFunc = ReleaseFunction();
	}
};

template<typename Type>
class KReferenceHolder
{
public:
	typedef std::function<void(Type)> ReleaseFunction;
protected:
	KReference<Type> m_Ref;
	uint32_t* m_RefCount;	
	void DecreaseReference()
	{
		if (m_RefCount && --*m_RefCount == 0)
		{
			m_Ref.Release();
			SAFE_DELETE(m_RefCount);
		}
	}
public:
	KReferenceHolder()
		: m_RefCount(nullptr)
	{
	}
	KReferenceHolder(Type soul, ReleaseFunction releaseFunc)
		: m_Ref(soul, releaseFunc)
		, m_RefCount(new uint32_t(1))
	{
	}
	~KReferenceHolder()
	{
		DecreaseReference();
	}
	KReferenceHolder(const KReferenceHolder& rhs)
	{
		m_Ref = rhs.m_Ref;
		m_RefCount = rhs.m_RefCount;
		if (m_RefCount)
			++*m_RefCount;
	}
	KReferenceHolder(KReferenceHolder&& rhs)
	{
		m_Ref = std::move(rhs.m_Ref);
		m_RefCount = std::move(rhs.m_RefCount);
		rhs.m_RefCount = nullptr;
	}
	KReferenceHolder& operator=(const KReferenceHolder& rhs)
	{
		DecreaseReference();
		m_Ref = rhs.m_Ref;
		m_RefCount = rhs.m_RefCount;
		if (m_RefCount)
			++* m_RefCount;
		return *this;
	}
	KReferenceHolder& operator=(KReferenceHolder&& rhs)
	{
		DecreaseReference();
		m_Ref = std::move(rhs.m_Ref);
		m_RefCount = std::move(rhs.m_RefCount);
		rhs.m_RefCount = nullptr;
		return *this;
	}
	const Type& Get() const
	{
		return m_Ref.Get();
	}
	const Type& operator*() const
	{
		return m_Ref.Get();
	}
	const Type& operator->() const
	{
		return m_Ref.Get();
	}
	Type& Get()
	{
		return m_Ref.Get();
	}
	Type& operator*()
	{
		return m_Ref.Get();
	}
	Type& operator->()
	{
		return m_Ref.Get();
	}
	uint32_t GetRefCount() const
	{
		return m_RefCount ? *m_RefCount : 0;
	}
	explicit operator bool() const
	{
		return m_RefCount ? (*m_RefCount > 0) : false;
	}
	void Release()
	{
		DecreaseReference();
		m_Ref = KReference<Type>();
		m_RefCount = nullptr;
	}
};