#include "KRender/Internal/KBufferBase.h"
#include "KRender/Interface/IKTexture.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderCommand.h"
#include "KRender/Publish/KCamera.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKJson.h"
#include "KBase/Publish/KInteger.h"
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <utility>

static_assert(std::is_same<decltype(std::declval<IKVertexBuffer&>().GetVertexCount()), uint32_t>::value, "vertex count");
static_assert(std::is_same<decltype(std::declval<IKVertexBuffer&>().GetBufferSize()), uint64_t>::value, "buffer bytes");
static_assert(std::is_same<decltype(&IKVertexBuffer::InitMemory), bool (IKVertexBuffer::*)(uint32_t, uint32_t, const void*)>::value, "vertex initialization");
static_assert(std::is_same<decltype(&IKRenderWindow::GetPosition), bool (IKRenderWindow::*)(int32_t&, int32_t&)>::value, "signed coordinates");
static_assert(std::is_same<decltype(std::declval<IKTexture&>().GetWidth()), uint32_t>::value, "texture width");
static_assert(std::is_same<decltype(std::declval<IKJsonValue&>().Size()), uint32_t>::value, "JSON count");
static_assert(std::is_same<IKEntity::IDType, uint64_t>::value, "entity ID");
static_assert(std::is_same<decltype(KDynamicConstantBufferUsage::offset), uint32_t>::value, "dynamic offset");
static_assert(std::is_same<decltype(KInstanceBufferUsage::offset), uint64_t>::value, "instance byte offset");
static_assert(std::is_same<decltype(KCodecResult::uMipmap), uint32_t>::value, "mip count");
static_assert(std::is_same<decltype(KSubImageInfo::uOffset), size_t>::value, "host image byte offset");

struct CpuVertexBuffer : KVertexBufferBase
{
	bool InitDevice(bool hostVisible) override { return false; }
	bool SetDebugName(const char* pName) override { return false; }
	bool DiscardMemory() override { return false; }
	bool Map(void** ppData, MapAccess access) override { return false; }
	bool UnMap() override { return false; }
	bool Write(const void* pData) override { return false; }
	bool Write(const void* pData, uint64_t offset, uint64_t size) override { return false; }
	bool Read(void* pData) override { return false; }
	bool Read(void* pData, uint64_t offset, uint64_t size) override { return false; }
	bool CopyFrom(IKVertexBufferPtr pSource) override { return false; }
	bool CopyTo(IKVertexBufferPtr pDest) override { return false; }
};

static unsigned checks = 0;
#define CHECK(expr) do { ++checks; if (!(expr)) throw std::runtime_error(#expr); } while (false)

int main()
{
	try
	{
		CHECK(KInteger::FitsUInt32(UINT32_MAX));
		CHECK(!KInteger::FitsUInt32(uint64_t(UINT32_MAX) + 1));
		CHECK(KInteger::FitsMemorySize(SIZE_MAX));
#if SIZE_MAX == UINT32_MAX
		CHECK(!KInteger::FitsMemorySize(uint64_t(SIZE_MAX) + 1));
#endif

		CpuVertexBuffer buffer;
		const float vertices[] = { 1, 2, 3, 4, 5, 6 };
		CHECK(buffer.InitMemory(2, sizeof(float) * 3, vertices));
		CHECK(buffer.GetVertexCount() == 2 && buffer.GetVertexSize() == 12 && buffer.GetBufferSize() == 24);
		CHECK(!buffer.InitMemory(UINT32_MAX, UINT32_MAX, nullptr));
		CHECK(buffer.GetVertexCount() == 2 && buffer.GetBufferSize() == 24);
		CHECK(buffer.UnInit());

		size_t bytes = 0;
#if SIZE_MAX > UINT32_MAX
		CHECK(KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 65536, 65536, 1, bytes));
		CHECK(uint64_t(bytes) == UINT64_C(17179869184));
#else
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 65536, 65536, 1, bytes) && bytes == 0);
#endif
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R32G32B32A32_FLOAT, UINT32_MAX, UINT32_MAX, UINT32_MAX, bytes) && bytes == 0);

		KCamera camera;
		glm::vec3 origin, direction;
		CHECK(!camera.CalcPickRay(-1, 0, 640, 480, origin, direction));
		CHECK(!camera.CalcPickRay(0, -1, 640, 480, origin, direction));
		CHECK(!camera.CalcPickRay(0, 0, 0, 480, origin, direction));
		CHECK(camera.CalcPickRay(320, 240, 640, 480, origin, direction));

		std::printf("Size type regression passed (%u checks, %zu-bit host).\n", checks, sizeof(size_t) * 8);
		return 0;
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "Size type regression failed: %s\n", error.what());
		return 1;
	}
}
