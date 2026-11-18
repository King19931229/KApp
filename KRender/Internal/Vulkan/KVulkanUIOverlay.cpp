#include "KVulkanUIOverlay.h"

#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanRenderPass.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"

#include "Internal/KRenderGlobal.h"

#include "imgui.h"
#include "imgui_internal.h"

KVulkanUIOverlay::KVulkanUIOverlay()
{
}

KVulkanUIOverlay::~KVulkanUIOverlay()
{
}

bool KVulkanUIOverlay::Draw(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	unsigned int imageIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	ImDrawData* drawData = &((ImGuiContext*)m_UIContext)->Viewports[0]->DrawDataP;
	if (!drawData->Valid || drawData->CmdListsCount == 0)
	{
		return true;
	}

	struct DrawSnapshot
	{
		ImVec2 displayPos, displaySize, framebufferScale;
		std::vector<std::shared_ptr<ImDrawList>> lists;
		std::unordered_map<uint64_t, TextureBinding> textures;
	};
	auto snapshot = std::make_shared<DrawSnapshot>();
	snapshot->displayPos = drawData->DisplayPos;
	snapshot->displaySize = drawData->DisplaySize;
	snapshot->framebufferScale = drawData->FramebufferScale;
	snapshot->textures = m_Textures;
	snapshot->textures[0] = { m_FontTexture->GetFrameBuffer(), m_FontSampler };
	for (int n = 0; n < drawData->CmdListsCount; ++n)
	{
		const ImDrawList* source = drawData->CmdLists[n];
		ImDrawList* copy = source->CloneOutput();
		copy->_CallbacksDataBuf = source->_CallbacksDataBuf;
		for (ImDrawCmd& cmd : copy->CmdBuffer)
		{
			if (cmd.UserCallback && cmd.UserCallbackDataOffset >= 0 && cmd.UserCallbackDataSize > 0)
				cmd.UserCallbackData = copy->_CallbacksDataBuf.Data + cmd.UserCallbackDataOffset;
		}
		snapshot->lists.emplace_back(copy, [](ImDrawList* list) { IM_DELETE(list); });
	}
	IKPipelinePtr pipeline = m_Pipeline;
	IKVertexBufferPtr vertexBuffer = m_VertexBuffers[imageIndex];
	IKIndexBufferPtr indexBuffer = m_IndexBuffers[imageIndex];

	commandList.AddLowLevelRenderJob([snapshot, renderPass, pipeline, vertexBuffer, indexBuffer](IKCommandBufferPtr inCommandBuffer)
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = (KVulkanCommandBuffer*)inCommandBuffer.get();
		const VkCommandBuffer commandBuffer = vulkanCommandBuffer->GetVkHandle();
		int32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)pipeline.get();
		KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)vertexBuffer.get();
		KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)indexBuffer.get();

		IKPipelineHandlePtr pipelineHandle = nullptr;
		if (vulkanPipeline->GetHandle(renderPass, pipelineHandle))
		{
			auto* vulkanHandle = static_cast<KVulkanPipelineHandle*>(pipelineHandle.get());
			VkPipeline vkPipeline = vulkanHandle->GetVkPipeline();
			const PrimitiveTopology topology = vulkanHandle->GetPrimitiveTopology();

			VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
			VkBuffer vkVertexBufferHandle = vulkanVertexBuffer->GetVulkanHandle();
			if (snapshot->displaySize.x <= 0.0f || snapshot->displaySize.y <= 0.0f) return;
			const int framebufferWidth = (int)(snapshot->displaySize.x * snapshot->framebufferScale.x);
			const int framebufferHeight = (int)(snapshot->displaySize.y * snapshot->framebufferScale.y);
			PushConstBlock push;
			push.scale = glm::vec2(2.0f / snapshot->displaySize.x, 2.0f / snapshot->displaySize.y);
			push.translate = glm::vec2(-1.0f - snapshot->displayPos.x * push.scale.x, -1.0f - snapshot->displayPos.y * push.scale.y);
			auto setupState = [&]()
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkVertexBufferHandle, offsets);
				vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());
			};
			setupState();

			for (const auto& list : snapshot->lists)
			{
				const ImDrawList* cmd_list = list.get();
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					if (pcmd->UserCallback)
					{
						if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) setupState();
						else pcmd->UserCallback(cmd_list, pcmd);
						continue;
					}
					auto texture = snapshot->textures.find((uint64_t)pcmd->GetTexID());
					if (texture == snapshot->textures.end()) continue;
					int minX = std::max(0, (int)((pcmd->ClipRect.x - snapshot->displayPos.x) * snapshot->framebufferScale.x));
					int minY = std::max(0, (int)((pcmd->ClipRect.y - snapshot->displayPos.y) * snapshot->framebufferScale.y));
					int maxX = std::min(framebufferWidth, (int)((pcmd->ClipRect.z - snapshot->displayPos.x) * snapshot->framebufferScale.x));
					int maxY = std::min(framebufferHeight, (int)((pcmd->ClipRect.w - snapshot->displayPos.y) * snapshot->framebufferScale.y));
					if (maxX <= minX || maxY <= minY) continue;
					VkDescriptorSet descriptorSet = vulkanPipeline->AllocDescriptorSet(0, vulkanCommandBuffer->GetVkPool(), nullptr, 0, nullptr, 0);
					if (descriptorSet == VK_NULL_HANDLE) continue;
					VkDescriptorImageInfo imageInfo = {};
					imageInfo.sampler = static_cast<KVulkanSampler*>(texture->second.sampler.get())->GetVkSampler();
					imageInfo.imageView = static_cast<KVulkanFrameBuffer*>(texture->second.image.get())->GetImageView();
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					VkWriteDescriptorSet write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSet;
					write.dstBinding = SHADER_BINDING_TEXTURE0;
					write.descriptorCount = 1;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.pImageInfo = &imageInfo;
					vkUpdateDescriptorSets(KVulkanGlobal::device, 1, &write, 0, nullptr);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
					VkRect2D scissorRect;
					scissorRect.offset = { minX, minY };
					scissorRect.extent = { (uint32_t)(maxX - minX), (uint32_t)(maxY - minY) };
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vulkanCommandBuffer->DrawIndexed(topology, pcmd->ElemCount, 1, indexOffset + pcmd->IdxOffset, vertexOffset + pcmd->VtxOffset, 0);
				}
				indexOffset += cmd_list->IdxBuffer.Size;
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}
	});

	return true;
}