#pragma once

#include "KBase/Publish/KConfig.h"
#include <functional>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <assert.h>

template<typename Type, bool UniqueCheck>
class KReferenceUniqueCheck
{
};

template<typename Type>
struct KReferenceUniqueCheckData
{
	std::unordered_set<Type> addedMap;
	std::mutex uniqueMutex;
};

template<typename Type>
class KReferenceUniqueCheck<Type, true>
{
protected:
	static KReferenceUniqueCheckData<Type> ms_CheckData;

	void AddUnqiueCheck(Type element)
	{
		std::unique_lock<std::mutex> lock(ms_CheckData.uniqueMutex);
		if (ms_CheckData.addedMap.find(element) != ms_CheckData.addedMap.end())
		{
			exit(0);
		}
		ms_CheckData.addedMap.insert(element);
	}

	void RemoveUnqiueCheck(Type element)
	{
		std::unique_lock<std::mutex> lock(ms_CheckData.uniqueMutex);
		if (ms_CheckData.addedMap.find(element) == ms_CheckData.addedMap.end())
		{
			exit(0);
		}
		ms_CheckData.addedMap.erase(element);
	}
};

template<typename Type>
class KReferenceUniqueCheck<Type, false>
{
protected:
	void AddUnqiueCheck(Type element) {}
	void RemoveUnqiueCheck(Type element) {}
};

template<typename Type, bool UniqueCheck = false>
class KReference : KReferenceUniqueCheck<Type, UniqueCheck>
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
		AddUnqiueCheck(m_Soul);
		ASSERT_RESULT(m_Soul);
	}
	KReference(Type soul, ReleaseFunction releaseFunc)
		: m_Soul(soul)
		, m_ReleaseFunc(releaseFunc)
	{
		AddUnqiueCheck(m_Soul);
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
		if (this != &rhs)
		{
			m_Soul = std::move(rhs.m_Soul);
			m_ReleaseFunc = std::move(rhs.m_ReleaseFunc);
			rhs.m_Soul = nullptr;
			rhs.m_ReleaseFunc = ReleaseFunction();
		}
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
	void __Release__()
	{
		RemoveUnqiueCheck(m_Soul);
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
	std::atomic_uint32_t atomicRefCounter;
	KReferenceHolderRefCounter(uint32_t counter)
		: atomicRefCounter(counter)
	{
	}
	KReferenceHolderRefCounter(const KReferenceHolderRefCounter& rhs)
	{
		atomicRefCounter.store(rhs.atomicRefCounter.load());
	}
	KReferenceHolderRefCounter(KReferenceHolderRefCounter&& rhs)
	{
		atomicRefCounter.store(rhs.atomicRefCounter.load());
	}
	operator uint32_t()
	{
		return atomicRefCounter.load();
	}
	uint32_t operator++()
	{
		atomicRefCounter.fetch_add(1);
		return atomicRefCounter.load();
	}
	uint32_t operator++(int)
	{
		return atomicRefCounter.fetch_add(1);
	}
	uint32_t operator--()
	{
		atomicRefCounter.fetch_sub(1);
		return atomicRefCounter.load();
	}
	uint32_t operator--(int)
	{
		return atomicRefCounter.fetch_sub(1);
	}
};

template<>
struct KReferenceHolderRefCounter<false>
{
	uint32_t refCounter;
	KReferenceHolderRefCounter(uint32_t counter)
		: refCounter(counter)
	{}
	KReferenceHolderRefCounter(const KReferenceHolderRefCounter& rhs)
	{
		refCounter = rhs.refCounter;
	}
	KReferenceHolderRefCounter(KReferenceHolderRefCounter&& rhs)
	{
		refCounter = rhs.refCounter;
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

template<typename Type, bool MultiThread = true, bool UniqueCheck = false>
class KReferenceHolder
{
public:
	typedef std::function<void(Type)> ReleaseFunction;
protected:
	typedef KReferenceHolderRefCounter<MultiThread> RefCounterType;
	KReference<Type, UniqueCheck> m_Ref;
	RefCounterType* m_RefCount;
	void DecreaseReference()
	{
		if (m_RefCount && (*m_RefCount)-- == 1)
		{
			m_Ref.__Release__();
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
		if (this != &rhs)
		{
			DecreaseReference();
			m_Ref = std::move(rhs.m_Ref);
			m_RefCount = std::move(rhs.m_RefCount);
			rhs.m_RefCount = nullptr;
		}
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
		m_Ref = KReference<Type, UniqueCheck>();
		m_RefCount = nullptr;
	}
};