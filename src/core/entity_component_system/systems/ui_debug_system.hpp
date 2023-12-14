#pragma once

struct ui_debug_system : public entity_system
{
	system_constructor(ui_debug_system)
	{
	}

	void Update(global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCompCullingCommonData)
	{
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(250, 300));
		ImGui::Begin("Renderer settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		ImGui::Checkbox("Enable light source shadows", (bool*)&WorldUpdate.LightSourceShadowsEnabled);
		ImGui::Checkbox("Enable frustrum culling", (bool*)&MeshCompCullingCommonData.FrustrumCullingEnabled);
		ImGui::Checkbox("Enable occlusion culling", (bool*)&MeshCompCullingCommonData.OcclusionCullingEnabled);
		ImGui::SliderFloat("Global Light Size", &WorldUpdate.GlobalLightSize, 0.0f, 1.0f);

		ImGui::End();

		ImGui::Render();
	}
};
