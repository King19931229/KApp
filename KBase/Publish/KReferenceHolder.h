#pragma once

#include "KBase/Publish/KConfig.h"
#include <functional>
#include <atomic>
#include <assert.h>

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
	explicit KReference(Type soul)
		: m_Soul(soul)
		, m_ReleaseFunc([](Type soul) { SAFE_DELETE(soul); })
	{
		ASSERT_RESULT(m_Soul);
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
	bool operator==(const KReference& rhs) const
	{
		return m_Soul == rhs.m_Soul;
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

template<bool MultiThread>
struct KReferenceHolderRefCounter
{
};

template<>
struct KReferenceHolderRefCounter<true>
{
	std::atomic_uint32_t refCounter;
	KReferenceHolderRefCounter(uint32_t counter)
		: refCounter(counter)
	{
	}
	KReferenceHolderRefCounter(KReferenceHolderRefCounter& rhs)
	{
		refCounter.store(rhs.refCounter.load());
	}
	KReferenceHolderRefCounter(KReferenceHolderRefCounter&& rhs)
	{
		refCounter.store(rhs.refCounter.load());
	}
	uint32_t operator=(KReferenceHolderRefCounter&& rhs)
	{
		refCounter.store(rhs.refCounter.load());
		return refCounter.load();
	}
	operator uint32_t()
	{
		return refCounter.load();
	}
	uint32_t operator++()
	{
		refCounter.fetch_add(1);
		return refCounter.load();
	}
	uint32_t operator++(int)
	{
		uint32_t counter = refCounter.load();
		refCounter.fetch_add(1);
		return counter;
	}
	uint32_t operator--()
	{
		refCounter.fetch_sub(1);
		return refCounter.load();
	}
	uint32_t operator--(int)
	{
		uint32_t counter = refCounter.load();
		refCounter.fetch_sub(1);
		return counter;
	}
};

template<>
struct KReferenceHolderRefCounter<false>
{
	uint32_t refCounter;
	KReferenceHolderRefCounter(uint32_t counter)
		: refCounter(counter)
	{}
	KReferenceHolderRefCounter(KReferenceHolderRefCounter& rhs)
	{
		refCounter = rhs.refCounter;
	}
	KReferenceHolderRefCounter(KReferenceHolderRefCounter&& rhs)
	{
		refCounter = rhs.refCounter;
	}
	uint32_t operator=(KReferenceHolderRefCounter&& rhs)
	{
		refCounter = rhs.refCounter;
		return refCounter;
	}
	operator uint32_t()
	{
		return refCounter;
	}
	uint32_t operator++()
	{
		return ++refCounter;
	}
	uint32_t operator++(int)
	{
		return refCounter++;
	}
	uint32_t operator--()
	{
		return --refCounter;
	}
	uint32_t operator--(int)
	{
		return refCounter--;
	}
};

template<typename Type, bool MultiThread = true>
class KReferenceHolder
{
public:
	typedef std::function<void(Type)> ReleaseFunction;
protected:
	typedef KReferenceHolderRefCounter<MultiThread> RefCounterType;
	KReference<Type> m_Ref;
	RefCounterType* m_RefCount;
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
	explicit KReferenceHolder(Type soul)
		: m_Ref(soul)
		, m_RefCount(KNEW RefCounterType(1))
	{
	}
	KReferenceHolder(Type soul, ReleaseFunction releaseFunc)
		: m_Ref(soul, releaseFunc)
		, m_RefCount(KNEW RefCounterType(1))
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
	bool operator==(const KReferenceHolder& rhs) const
	{
		return (m_Ref == rhs.m_Ref) && (m_RefCount == rhs.m_RefCount);
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

/*
template<typename Type>
struct std::hash<KReferenceHolder<Type>>
{
	inline std::size_t operator()(const KReferenceHolder<Type>& holder) const
	{
		return (size_t)holder.Get();
	}
};
*/