#pragma once

struct scene_manager
{
	struct scene_info
	{
		library_block Module;
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

	bool DebugMeshesDrawEnabled = true;

	scene_manager(window& Window, std::string ScenesPath = "..\\build\\scenes\\")
	{
		WorldUpdate = {};
		MeshCompCullingCommonData = {};

		LoadAllScenes(ScenesPath);
	}

	// TODO: Load a scenes by their file names. Load and unload if scene was recompiled(many if needed, but for this I need to check this every times in the loop I guess but that is a lot of work and think about something better)
	void LoadScene(const std::filesystem::directory_entry& SceneCode);
	void LoadAllScenes(std::string ScenesPath = "..\\build\\scenes\\");
	void UpdateScenes(std::string ScenesPath = "..\\build\\scenes\\");

	bool IsCurrentSceneInitialized(){return Infos[CurrentScene].IsInitialized;}

	void StartScene(window& Window);
	void UpdateScene(window& Window, global_pipeline_context* PipelineContext, alloc_vector<light_source>& GlobalLightSources);

	void RenderScene(window& Window, global_pipeline_context* PipelineContext,
					 alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility, 
					 alloc_vector<mesh_draw_command>& DynamicDebugInstances, alloc_vector<u32>& DynamicDebugVisibility,
					 alloc_vector<light_source>& GlobalLightSources);
	void RenderUI();

	void SaveSceneStateToFile(const char* FilePath);
	void LoadSceneStateFromFile(const char* FilePath);
};
