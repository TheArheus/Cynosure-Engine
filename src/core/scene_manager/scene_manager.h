#pragma once

struct scene_manager
{
	struct scene_info
	{
		HMODULE Module;
		std::string Name;
		std::string Path;
		std::filesystem::file_time_type LastFileCreation;
		bool IsInitialized;
	};

	global_world_data WorldUpdate;
	mesh_comp_culling_common_input MeshCompCullingCommonData;

	u32 CurrentScene = 0;
	std::vector<std::unique_ptr<scene>> Scenes;
	std::vector<scene_info> Infos;

	mesh GlobalGeometries;
	mesh GlobalDebugGeometries;

	texture GfxColorTarget;
	texture GfxDepthTarget;
	texture DebugCameraViewDepthTarget;

	global_pipeline_context PipelineContext;
	memory_heap GlobalHeap;

	bool DebugMeshesDrawEnabled = true;

	scene_manager(window& Window, std::string ScenesPath = "..\\build\\scenes\\") : 
		PipelineContext(Window.Gfx)
	{
		WorldUpdate = {};
		MeshCompCullingCommonData = {};

		GlobalHeap.CreateResource(Window.Gfx);

		LoadAllScenes(ScenesPath);

		texture::input_data TextureInputData = {};
		TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
		TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Layers    = 1;
		TextureInputData.Format    = Window.Gfx->SurfaceFormat.format;
		TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		GfxColorTarget = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData);
		TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
		TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		GfxDepthTarget = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData);
		DebugCameraViewDepthTarget = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData);
	}

	// TODO: Load a scenes by their file names. Load and unload if scene was recompiled(many if needed, but for this I need to check this every times in the loop I guess but that is a lot of work and think about something better)
	void LoadScene(const std::filesystem::directory_entry& SceneCode);
	void LoadAllScenes(std::string ScenesPath = "..\\build\\scenes\\");
	void UpdateScenes(std::string ScenesPath = "..\\build\\scenes\\");

	bool IsCurrentSceneInitialized(){return Infos[CurrentScene].IsInitialized;}

	void StartScene(window& Window);
	void UpdateScene(window& Window,
					 alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility, 
					 alloc_vector<mesh_draw_command>& DynamicDebugInstances, alloc_vector<u32>& DynamicDebugVisibility,
					 alloc_vector<light_source>& GlobalLightSources);

	void SaveSceneStateToFile(const char* FilePath);
	void LoadSceneStateFromFile(const char* FilePath);
};
