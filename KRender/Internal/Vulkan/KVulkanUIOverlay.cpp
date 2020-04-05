#include "KVulkanUIOverlay.h"

#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanCommandBuffer.h"

#include "Internal/KRenderGlobal.h"

#include "imgui.h"

KVulkanUIOverlay::KVulkanUIOverlay()
{

}

KVulkanUIOverlay::~KVulkanUIOverlay()
{

}

bool KVulkanUIOverlay::Draw(unsigned int imageIndex, IKRenderTargetPtr target, IKCommandBufferPtr commandBuffer)
{
	if(imageIndex < m_Pipelines.size())
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = (KVulkanCommandBuffer*)commandBuffer.get();
		const VkCommandBuffer commandBuffer = vulkanCommandBuffer->GetVkHandle();

		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if ((!imDrawData) || (imDrawData->CmdListsCount == 0))
		{
			return true;
		}

		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_Pipelines[imageIndex].get();
		KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_VertexBuffers[imageIndex].get();
		KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_IndexBuffers[imageIndex].get();
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)(target.get());

		IKPipelineHandlePtr pipelineHandle = nullptr;
		if (m_Pipelines[imageIndex]->GetHandle(target, pipelineHandle))
		{
			VkPipeline pipeline = ((KVulkanPipelineHandle*)pipelineHandle.get())->GetVkPipeline();

			VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
			VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

			VkBuffer vkVertexBufferHandle = vulkanVertexBuffer->GetVulkanHandle();

			ImGuiIO& io = ImGui::GetIO();

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			m_PushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
			m_PushConstBlock.translate = glm::vec2(-1.0f);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &m_PushConstBlock);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkVertexBufferHandle, offsets);
			vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}

		return true;
	}
	return false;
}