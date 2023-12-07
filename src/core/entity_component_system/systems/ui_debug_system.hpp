#pragma once

struct ui_debug_system : public entity_system
{
	system_constructor(ui_debug_system)
	{
	}

	void Update(global_pipeline_context& PipelineContext, texture& GfxColorTarget, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCompCullingCommonData)
	{
		ImGui_ImplVulkan_NewFrame();

		VkRenderingInfoKHR RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
		RenderingInfo.renderArea = {{}, {u32(GfxColorTarget.Width), u32(GfxColorTarget.Height)}};
		RenderingInfo.layerCount = 1;
		RenderingInfo.viewMask   = 0;

		VkRenderingAttachmentInfoKHR ColorInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR};
		ColorInfo.imageView = GfxColorTarget.Views[0];
		ColorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		ColorInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorInfo.clearValue = {0, 0, 0, 0};
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &ColorInfo;

		vkCmdBeginRenderingKHR(*PipelineContext.CommandList, &RenderingInfo);

		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(500, 500));
		ImGui::Begin("Renderer settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		ImGui::Checkbox("Enable light source shadows", (bool*)&WorldUpdate.LightSourceShadowsEnabled);
		ImGui::Checkbox("Enable frustrum culling", (bool*)&MeshCompCullingCommonData.FrustrumCullingEnabled);
		ImGui::Checkbox("Enable occlusion culling", (bool*)&MeshCompCullingCommonData.OcclusionCullingEnabled);

		ImGui::End();

		ImGui::Render();

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *PipelineContext.CommandList);

		vkCmdEndRenderingKHR(*PipelineContext.CommandList);
	}
};
