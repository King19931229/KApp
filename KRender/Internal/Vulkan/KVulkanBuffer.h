#pragma once
#include "Internal/KBufferBase.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"
#include <atomic>

struct KVulkanBufferTicket;
class KVulkanCommandBuffer;

class KVulkanStageBuffer
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	uint64_t m_BufferSize;
	bool m_Mapping;
public:
	KVulkanStageBuffer();
	~KVulkanStageBuffer();

	bool Init(uint64_t bufferSize);
	bool UnInit();

	bool Map(void** ppData);
	bool UnMap();
	bool Write(const void* pData);
	bool Write(const void* pData, uint64_t size);
	bool Read(void* pData);

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
};

class KVulkanBuffer
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	VkBufferUsageFlags m_Usages;
	KVulkanStageBuffer m_StageBuffer;
	std::string m_DebugName;
	uint64_t m_BufferSize;
	uint32_t m_UniqueID;
	bool m_bHostVisible;
	bool m_Mapping;
	MapAccess m_MapAccess;

	static std::atomic_uint32_t ms_UniqueIDCounter;
public:
	KVulkanBuffer();
	~KVulkanBuffer();

	bool InitDevice(VkBufferUsageFlags usages, const void* pData, uint64_t bufferSize, bool hostVisible);
	bool UnInit();

	bool SetDebugName(const char* pName);
	inline const std::string& GetDebugName() const { return m_DebugName; }

	bool Map(void** ppData, MapAccess access = MA_READ_WRITE);
	bool UnMap();
	bool Write(const void* pData);
	bool Write(const void* pData, uint64_t offset, uint64_t size);
	bool RecordUpload(VkCommandBuffer commandBuffer, const void* data, uint64_t size);
	bool Read(void* pData);
	bool Read(void* pData, uint64_t offset, uint64_t size);

	inline bool IsHostVisible() const { return m_bHostVisible; }
	inline uint64_t GetBufferSize() const { return m_BufferSize; }
	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
	inline uint32_t GetUniqueID() const { return m_UniqueID; }

	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanVertexBuffer : public KVertexBufferBase
{
	friend class KVulkanStorageBuffer;
protected:
	KVulkanBuffer m_Buffer;
public:
	KVulkanVertexBuffer();
	~KVulkanVertexBuffer();

	virtual bool InitDevice(bool hostVisible);
	virtual bool UnInit();

	virtual bool SetDebugName(const char* pName);

	virtual bool DiscardMemory();

	virtual bool Map(void** ppData, MapAccess access = MA_READ_WRITE);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Write(const void* pData, uint64_t offset, uint64_t size);
	virtual bool Read(void* pData);
	virtual bool Read(void* pData, uint64_t offset, uint64_t size);

	virtual bool CopyFrom(IKVertexBufferPtr pSource);
	virtual bool CopyTo(IKVertexBufferPtr pDest);

	VkBuffer GetVulkanHandle();
	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanIndexBuffer : public KIndexBufferBase
{
protected:
	KVulkanBuffer m_Buffer;
public:
	KVulkanIndexBuffer();
	virtual ~KVulkanIndexBuffer();

	virtual bool InitDevice(bool hostVisible);
	virtual bool UnInit();

	virtual bool SetDebugName(const char* pName);

	virtual bool DiscardMemory();

	virtual bool Map(void** ppData, MapAccess access = MA_READ_WRITE);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Write(const void* pData, uint64_t offset, uint64_t size);
	virtual bool Read(void* pData);
	virtual bool Read(void* pData, uint64_t offset, uint64_t size);

	virtual bool IsHostVisible() const;

	virtual bool CopyFrom(IKIndexBufferPtr pSource);
	virtual bool CopyTo(IKIndexBufferPtr pDest);

	inline VkIndexType GetVulkanIndexType() { return m_IndexType == IT_16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32; }

	VkBuffer GetVulkanHandle();
	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanBufferBase
{
	VkBufferUsageFlagBits m_Usage;
protected:
	KVulkanBuffer m_Buffer;
	KVulkanBufferTicket* m_Ticket = nullptr;
	KVulkanBuffer* m_MappedBuffer = nullptr;

	explicit KVulkanBufferBase(VkBufferUsageFlagBits usage);
	~KVulkanBufferBase() = default;

	bool InitBuffer(bool hostVisible, bool persistent, VkBufferUsageFlags extraUsages = 0);
	bool UnInitBuffer();
	virtual KVulkanBuffer& GetFixedBuffer() { return m_Buffer; }
	virtual uint64_t GetLogicalSize() const = 0;
	virtual const std::vector<char>& GetInitialData() const = 0;
	KVulkanBuffer* GetMemoryBuffer(bool readback = false);
	KVulkanBuffer* GetVulkanBuffer();

	bool SetDebugName(const char* pName);
	bool Map(void** ppData, MapAccess access);
	bool UnMap();
	bool Write(const void* pData);
	bool Write(const void* pData, uint64_t offset, uint64_t size);
	bool Read(void* pData);
	bool Read(void* pData, uint64_t offset, uint64_t size);
	uint64_t GetBufferSize();
public:
	bool RecordUpload(KVulkanCommandBuffer& commandBuffer, const void* data, uint64_t offset, uint64_t size);
	VkBuffer GetVulkanHandle();
	uint32_t GetUniqueID() const;
};

class KVulkanStorageBuffer : public KStorageBufferBase, public KVulkanBufferBase
{
protected:
	std::shared_ptr<KVulkanBuffer> m_ReferencedBuffer;

	KVulkanBuffer& GetFixedBuffer() override;
	uint64_t GetLogicalSize() const override { return m_BufferSize; }
	const std::vector<char>& GetInitialData() const override { return m_Data; }
public:
	KVulkanStorageBuffer();

	bool InitFromVertexBuffer(IKVertexBufferPtr pSource) override;
	bool InitDevice(bool indirect, bool hostVisible, bool persistent = true) override;
	bool UnInit() override;

	bool SetDebugName(const char* pName) override { return KVulkanBufferBase::SetDebugName(pName); }
	uint64_t GetBufferSize() override { return KVulkanBufferBase::GetBufferSize(); }
	bool Map(void** ppData, MapAccess access = MA_READ_WRITE) override { return KVulkanBufferBase::Map(ppData, access); }
	bool UnMap() override { return KVulkanBufferBase::UnMap(); }
	bool Write(const void* pData) override { return KVulkanBufferBase::Write(pData); }
	bool Write(const void* pData, uint64_t offset, uint64_t size) override { return KVulkanBufferBase::Write(pData, offset, size); }
	bool Read(void* pData) override { return KVulkanBufferBase::Read(pData); }
	bool Read(void* pData, uint64_t offset, uint64_t size) override { return KVulkanBufferBase::Read(pData, offset, size); }

	bool CopyFrom(IKStorageBufferPtr pSource) override;
	bool CopyTo(IKStorageBufferPtr pDest) override;
};

class KVulkanUniformBuffer : public KUniformBufferBase, public KVulkanBufferBase
{
protected:
	uint64_t GetLogicalSize() const override { return m_BufferSize; }
	const std::vector<char>& GetInitialData() const override { return m_Data; }
public:
	KVulkanUniformBuffer();

	bool InitDevice(bool persistent = false) override;
	bool UnInit() override;

	bool SetDebugName(const char* pName) override { return KVulkanBufferBase::SetDebugName(pName); }
	uint64_t GetBufferSize() override { return KVulkanBufferBase::GetBufferSize(); }
	bool Map(void** ppData, MapAccess access = MA_READ_WRITE) override { return KVulkanBufferBase::Map(ppData, access); }
	bool UnMap() override { return KVulkanBufferBase::UnMap(); }
	bool Write(const void* pData) override { return KVulkanBufferBase::Write(pData); }
	bool Write(const void* pData, uint64_t offset, uint64_t size) override { return KVulkanBufferBase::Write(pData, offset, size); }
	bool Read(void* pData) override { return KVulkanBufferBase::Read(pData); }
	bool Read(void* pData, uint64_t offset, uint64_t size) override { return KVulkanBufferBase::Read(pData, offset, size); }

	bool Reference(void **ppData) override;
	bool CopyFrom(IKUniformBufferPtr pSource) override;
	bool CopyTo(IKUniformBufferPtr pDest) override;
};