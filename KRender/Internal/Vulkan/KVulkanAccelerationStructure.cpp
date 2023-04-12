#include "KVulkanAccelerationStructure.h"
#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "Internal/Asset/Material/KMaterialTextureBinding.h"
#include "KBase/Publish/KHash.h"
#include <unordered_map>

KVulkanAccelerationStructure::KVulkanAccelerationStructure()
	: m_IndexBuffer(nullptr)
	, m_VertexBuffer(nullptr)
	, m_TextureBinding(nullptr)
{
}

KVulkanAccelerationStructure::~KVulkanAccelerationStructure()
{
	ASSERT_RESULT(!m_IndexBuffer);
	ASSERT_RESULT(!m_VertexBuffer);
	ASSERT_RESULT(!m_TextureBinding);
}

bool KVulkanAccelerationStructure::InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer, IKMaterialTextureBinding* textureBinding)
{
	if (KVulkanGlobal::supportRaytrace)
	{
		if (format == VF_POINT_NORMAL_UV && vertexBuffer && indexBuffer)
		{
			KVulkanVertexBuffer* vulkanVertexBuffer = static_cast<KVulkanVertexBuffer*>(vertexBuffer.get());
			KVulkanIndexBuffer* vulkanIndexBuffer = static_cast<KVulkanIndexBuffer*>(indexBuffer.get());
			KMaterialTextureBinding* matTextureBinding = static_cast<KMaterialTextureBinding*>(textureBinding);

			m_IndexBuffer = vulkanIndexBuffer;
			m_VertexBuffer = vulkanVertexBuffer;
			m_TextureBinding = matTextureBinding;

			VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress = {};
			VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress = {};

			ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(vulkanVertexBuffer->GetVulkanHandle(), vertexBufferDeviceAddress.deviceAddress));
			ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(vulkanIndexBuffer->GetVulkanHandle(), indexBufferDeviceAddress.deviceAddress));

			// Always triangle list for now 
			uint32_t numTriangles = (uint32_t)vulkanIndexBuffer->GetIndexCount() / 3;

			// Build
			VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
			accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

			accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
			accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
			accelerationStructureGeometry.geometry.triangles.maxVertex = (uint32_t)vulkanVertexBuffer->GetVertexCount();
			accelerationStructureGeometry.geometry.triangles.vertexStride = (uint32_t)vulkanVertexBuffer->GetVertexSize();
			accelerationStructureGeometry.geometry.triangles.indexType = vulkanIndexBuffer->GetIndexType() == IT_32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
			accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
			accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
			accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

			// Get size info
			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
			accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
			accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			accelerationStructureBuildGeometryInfo.geometryCount = 1;
			accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
			accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

			KVulkanGlobal::vkGetAccelerationStructureBuildSizesKHR(
				KVulkanGlobal::device,
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&accelerationStructureBuildGeometryInfo,
				&numTriangles,
				&accelerationStructureBuildSizesInfo);

			KVulkanInitializer::CreateVkAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo, m_BottomUpAS);
			m_BottomUpAS.sizeInfo = accelerationStructureBuildSizesInfo;

			accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDEL;
			accelerationStructureBuildGeometryInfo.dstAccelerationStructure = m_BottomUpAS.handle;

			KVulkanInitializer::BuildBottomUpVkAccelerationStructure(accelerationStructureBuildGeometryInfo, accelerationStructureBuildSizesInfo, numTriangles);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool KVulkanAccelerationStructure::BuildTopDown(const std::vector<BottomASTransformTuple>& bottomASs, bool update)
{
	if (KVulkanGlobal::supportRaytrace)
	{
		vkDeviceWaitIdle(KVulkanGlobal::device);

		// if (m_Instances.size() != bottomASs.size())
		{
			update = false;
		}

		m_Instances.clear();
		m_Instances.reserve(bottomASs.size());

		std::vector<VkAccelerationStructureInstanceKHR> instances;
		instances.reserve(bottomASs.size());

		std::unordered_map<IKTexturePtr, uint32_t> texturesMap;
		std::unordered_map<uint32_t, uint32_t> mtlBuffersMap;
		m_Textures.clear();

		for (uint32_t objIndex = 0; objIndex < (uint32_t)bottomASs.size(); ++objIndex)
		{
			const BottomASTransformTuple& asTuple = bottomASs[objIndex];

			IKAccelerationStructurePtr as = std::get<0>(asTuple);
			const glm::mat4& transform = std::get<1>(asTuple);
			const glm::mat4 transformIT = glm::transpose(glm::inverse(transform));

			KVulkanAccelerationStructure* vulkanAS = static_cast<KVulkanAccelerationStructure*>(as.get());
			const KMaterialTextureBinding* textureBinding = vulkanAS->GetTextureBinding();

			KVulkanRayTraceMaterial material = {};
			material.diffuseTex = material.normalTex = material.specularTex = -1;
			material.placeholder = -1;
			if (textureBinding)
			{
				for (uint32_t i = 0; i < MTS_COUNT; ++i)
				{
					int32_t idx = -1;

					IKTexturePtr texture = textureBinding->GetTexture(i);
					IKSamplerPtr sampler = textureBinding->GetSampler(i);
					if (texture && sampler)
					{
						auto it = texturesMap.find(texture);
						if (it == texturesMap.end())
						{
							VkDescriptorImageInfo image = {};
							KVulkanTexture* vulkanTexture = static_cast<KVulkanTexture*>(texture.get());
							KVulkanSampler* vulkanSampler = static_cast<KVulkanSampler*>(sampler.get());
							image.imageView = vulkanTexture->GetImageView();
							image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							image.sampler = vulkanSampler->GetVkSampler();

							idx = (int32_t)m_Textures.size();
							texturesMap.insert({ texture,idx });
							m_Textures.push_back(std::move(image));
						}
						else
						{
							idx = it->second;
						}
					}

					if (i == MTS_DIFFUSE) material.diffuseTex = idx;
					if (i == MTS_SPECULAR_GLOSINESS) material.specularTex = idx;
					if (i == MTS_NORMAL) material.normalTex = idx;
				}
			}

			uint32_t mtlIndex = 0;

			uint32_t mtlHash = 0;
			KHash::HashCombine(mtlHash, material.diffuseTex);
			KHash::HashCombine(mtlHash, material.specularTex);
			KHash::HashCombine(mtlHash, material.normalTex);
			KHash::HashCombine(mtlHash, material.placeholder);

			MaterialBuffer materialBuffer;
			VkDeviceAddress materialAddress = VK_NULL_HANDEL;

			auto it = mtlBuffersMap.find(mtlHash);
			if (it == mtlBuffersMap.end())
			{
				// 创建材质Buffer
				KVulkanInitializer::CreateStorageBuffer(sizeof(KVulkanRayTraceMaterial), &material, materialBuffer.buffer, materialBuffer.allocInfo);
				mtlIndex = (uint32_t)m_Materials.size();
				m_Materials.push_back(materialBuffer);
				mtlBuffersMap.insert({ mtlHash, mtlIndex });
			}
			else
			{
				mtlIndex = it->second;
				materialBuffer = m_Materials[mtlIndex];
			}
			ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(materialBuffer.buffer, materialAddress));

			auto MatrixCheck = [](const glm::mat4& matrix)
			{
				for (uint32_t i = 0; i < 4; ++i)
				{
					for (uint32_t j = 0; j < 4; ++j)
					{
						if (isnan(matrix[j][i]) || isinf(matrix[j][i]))
						{
							return false;
						}
					}
				}
				return true;
			};

			assert(MatrixCheck(transform));
			assert(MatrixCheck(transformIT));

			// 创建场景Instance
			KVulkanRayTraceInstance rayInstance = {};
			rayInstance.transform = transform;
			rayInstance.transformIT = transformIT;
			rayInstance.objIndex = (uint32_t)objIndex;
			rayInstance.mtlIndex = (uint32_t)mtlIndex;
			rayInstance.materials = materialAddress;
			rayInstance.vertices = vulkanAS->GetVertexBuffer()->GetDeviceAddress();
			rayInstance.indices = vulkanAS->GetIndexBuffer()->GetDeviceAddress();
			m_Instances.push_back(rayInstance);

			// glm matrix column major
			VkTransformMatrixKHR transformMatrix =
			{
				transform[0][0], transform[1][0], transform[2][0], transform[3][0],
				transform[0][1], transform[1][1], transform[2][1], transform[3][1],
				transform[0][2], transform[1][2], transform[2][2], transform[3][2]
			};

			// https://nvpro-samples.github.io/vk_mini_path_tracer/extras.html#instancesandtransformationmatrices/transformationmatrices

			VkAccelerationStructureInstanceKHR instance = {};
			instance.transform = transformMatrix;
			instance.instanceCustomIndex = (uint32_t)objIndex;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instance.accelerationStructureReference = vulkanAS->GetBottomUp().deviceAddress;

			instances.push_back(instance);
		}

		// Buffer for instance data
		VkBuffer instanceBufferHandle = VK_NULL_HANDEL;
		KVulkanHeapAllocator::AllocInfo instanceAlloc;

		size_t instanceSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = instanceSize;
		// 以防instances为空
		bufferCreateInfo.size = std::max((size_t)bufferCreateInfo.size, sizeof(VkAccelerationStructureInstanceKHR));
		bufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferCreateInfo, nullptr, &instanceBufferHandle));

		VkMemoryRequirements memoryRequirements = {};
		vkGetBufferMemoryRequirements(KVulkanGlobal::device, instanceBufferHandle, &memoryRequirements);

		// Find memory requirements
		VkMemoryRequirements2			memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
		VkMemoryDedicatedRequirements	dedicatedRegs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
		VkBufferMemoryRequirementsInfo2	bufferReqs = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };

		memReqs.pNext = &dedicatedRegs;
		bufferReqs.buffer = instanceBufferHandle;

		vkGetBufferMemoryRequirements2(KVulkanGlobal::device, &bufferReqs, &memReqs);

		memoryRequirements = memReqs.memoryRequirements;

		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		uint32_t memoryTypeIndex = 0;
		ASSERT_RESULT(KVulkanHelper::FindMemoryType(KVulkanGlobal::physicalDevice,
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			memoryTypeIndex));

		ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferCreateInfo.usage, dedicatedRegs.requiresDedicatedAllocation, instanceAlloc));

		// 这里没有拷贝数据导致数据为空验证层居然在之后不报错
		if (instanceSize)
		{
			if (instanceAlloc.pMapped)
			{
				memcpy(instanceAlloc.pMapped, instances.data(), instanceSize);
			}
			else
			{
				void* instanceData = nullptr;
				VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, instanceAlloc.vkMemroy, instanceAlloc.vkOffset, instanceSize, 0, &instanceData));
				memcpy(instanceData, instances.data(), instanceSize);
				vkUnmapMemory(KVulkanGlobal::device, instanceAlloc.vkMemroy);
			}
		}
		VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, instanceBufferHandle, instanceAlloc.vkMemroy, instanceAlloc.vkOffset));

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(instanceBufferHandle, instanceDataDeviceAddress.deviceAddress));

		// Build
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = 1;
		uint32_t instance_count = (uint32_t)instances.size();

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		KVulkanGlobal::vkGetAccelerationStructureBuildSizesKHR(
			KVulkanGlobal::device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&primitive_count,
			&accelerationStructureBuildSizesInfo);

		if (accelerationStructureBuildSizesInfo.accelerationStructureSize != m_TopDownAS.sizeInfo.accelerationStructureSize)
		{
			update = false;
			accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		}

		if (!update)
		{
			if (m_TopDownAS.handle)
			{
				vkDestroyBuffer(KVulkanGlobal::device, m_TopDownAS.buffer, nullptr);
				m_TopDownAS.buffer = VK_NULL_HANDEL;
				KVulkanGlobal::vkDestroyAccelerationStructureKHR(KVulkanGlobal::device, m_TopDownAS.handle, nullptr);
				m_TopDownAS.handle = VK_NULL_HANDEL;
				KVulkanHeapAllocator::Free(m_TopDownAS.allocInfo);
			}
			KVulkanInitializer::CreateVkAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfo, m_TopDownAS);
			m_TopDownAS.sizeInfo = accelerationStructureBuildSizesInfo;
		}

		ASSERT_RESULT(accelerationStructureBuildSizesInfo.accelerationStructureSize <= m_TopDownAS.sizeInfo.accelerationStructureSize);

		if (update)
		{
			accelerationStructureBuildGeometryInfo.srcAccelerationStructure = m_TopDownAS.handle;
		}
		else
		{
			accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDEL;
		}
		accelerationStructureBuildGeometryInfo.dstAccelerationStructure = m_TopDownAS.handle;

		KVulkanInitializer::BuildTopDownVkAccelerationStructure(accelerationStructureBuildGeometryInfo, accelerationStructureBuildSizesInfo, instance_count);

		vkDestroyBuffer(KVulkanGlobal::device, instanceBufferHandle, nullptr);
		KVulkanHeapAllocator::Free(instanceAlloc);
	}
	return true;
}

bool KVulkanAccelerationStructure::InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs)
{
	return BuildTopDown(bottomASs, false);
}

bool KVulkanAccelerationStructure::UpdateTopDown(const std::vector<BottomASTransformTuple>& bottomASs)
{
	return BuildTopDown(bottomASs, true);
}

bool KVulkanAccelerationStructure::UnInit()
{
	if (KVulkanGlobal::supportRaytrace)
	{
		if (m_BottomUpAS.handle != VK_NULL_HANDEL)
		{
			vkDestroyBuffer(KVulkanGlobal::device, m_BottomUpAS.buffer, nullptr);
			m_BottomUpAS.buffer = VK_NULL_HANDEL;
			KVulkanGlobal::vkDestroyAccelerationStructureKHR(KVulkanGlobal::device, m_BottomUpAS.handle, nullptr);
			m_BottomUpAS.handle = VK_NULL_HANDEL;
			KVulkanHeapAllocator::Free(m_BottomUpAS.allocInfo);
		}
		m_BottomUpAS.deviceAddress = VK_NULL_HANDEL;

		if (m_TopDownAS.handle != VK_NULL_HANDEL)
		{
			vkDestroyBuffer(KVulkanGlobal::device, m_TopDownAS.buffer, nullptr);
			m_TopDownAS.buffer = VK_NULL_HANDEL;
			KVulkanGlobal::vkDestroyAccelerationStructureKHR(KVulkanGlobal::device, m_TopDownAS.handle, nullptr);
			m_TopDownAS.handle = VK_NULL_HANDEL;
			KVulkanHeapAllocator::Free(m_TopDownAS.allocInfo);
		}
		m_TopDownAS.deviceAddress = VK_NULL_HANDEL;

		for (MaterialBuffer& material : m_Materials)
		{
			KVulkanInitializer::DestroyStorageBuffer(material.buffer, material.allocInfo);
		}
		m_Materials.clear();
	}

	m_Materials.clear();
	m_Instances.clear();
	m_Textures.clear();
	m_IndexBuffer = nullptr;
	m_VertexBuffer = nullptr;
	m_TextureBinding = nullptr;

	return true;
}