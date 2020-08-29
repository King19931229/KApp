#pragma once
#include <mutex>
#include <queue>
#include <unordered_map>
#include "KBase/Publish/KConfig.h"

typedef unsigned int FrameGraphHandleType;

class KFrameGraphHandlePool
{
protected:
	std::mutex m_Lock;
	std::queue<FrameGraphHandleType> m_UnuseHandle;
	FrameGraphHandleType m_HandleCounter;
public:
	KFrameGraphHandlePool()
		: m_HandleCounter(1)
	{}

	~KFrameGraphHandlePool() {}

	void RecyleHandle(FrameGraphHandleType handle)
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		m_UnuseHandle.push(handle);
	}

	FrameGraphHandleType AcquireHandle()
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		if (!m_UnuseHandle.empty())
		{
			FrameGraphHandleType ret = m_UnuseHandle.front();
			m_UnuseHandle.pop();
			return ret;
		}
		return m_HandleCounter++;
	}
};

class KFrameGraphHandle
{
protected:
	unsigned int m_Handle;
	KFrameGraphHandlePool& m_Pool;
public:
	KFrameGraphHandle(KFrameGraphHandlePool& pool)
		: m_Pool(pool)
	{
		m_Handle = m_Pool.AcquireHandle();
	}

	~KFrameGraphHandle()
	{
		m_Pool.RecyleHandle(m_Handle);
	}

	unsigned int GetSoul() const { return m_Handle; }

	KFrameGraphHandle(const KFrameGraphHandle&) = delete;
	KFrameGraphHandle(KFrameGraphHandle&&) = delete;
	KFrameGraphHandle& operator=(const KFrameGraphHandle&) = delete;
	KFrameGraphHandle& operator=(KFrameGraphHandle&&) = delete;
};

typedef std::shared_ptr<KFrameGraphHandle> KFrameGraphHandlePtr;

class KFrameGraphID
{
protected:
	KFrameGraphHandlePtr m_HandlePtr;
public:
	KFrameGraphID(KFrameGraphHandlePool& pool)
		: m_HandlePtr(KNEW KFrameGraphHandle(pool))
	{}
	KFrameGraphID()
		: m_HandlePtr(nullptr)
	{}

	bool operator==(const KFrameGraphID& rhs) const
	{
		if (IsVaild() == rhs.IsVaild())
		{
			if (IsVaild())
			{
				return GetSoul() == rhs.GetSoul();
			}
			return true;
		}
		return false;
	}
	bool operator!=(const KFrameGraphID& rhs) const { return !(operator==(rhs)); }

	bool operator<(const KFrameGraphID& rhs) const
	{
		if (IsVaild() == rhs.IsVaild())
		{
			if (IsVaild())
			{
				return GetSoul() < rhs.GetSoul();
			}
		}
		return false;
	}
	bool operator>=(const KFrameGraphID& rhs) const { return !(operator<(rhs)); }

	bool operator>(const KFrameGraphID& rhs) const
	{
		if (IsVaild() == rhs.IsVaild())
		{
			if (IsVaild())
			{
				return GetSoul() < rhs.GetSoul();
			}
		}
		return false;
	}
	bool operator<=(const KFrameGraphID& rhs) const { return !(operator>(rhs)); }

	size_t GetSoul() const { return m_HandlePtr->GetSoul(); }
	bool IsVaild() const { return m_HandlePtr != nullptr; }	
};

template<>
struct std::hash<KFrameGraphID>
{
	inline std::size_t operator()(const KFrameGraphID& handle) const
	{
		if (handle.IsVaild())
			return handle.GetSoul();
		return 0;
	}
};

class KFrameGraphResource;
typedef std::shared_ptr<KFrameGraphResource> KFrameGraphResourcePtr;

typedef std::unordered_map<KFrameGraphID, KFrameGraphResourcePtr> ResourceMap;