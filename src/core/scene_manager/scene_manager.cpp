
void scene_manager::
LoadScene(const std::filesystem::directory_entry& SceneCode)
{
	std::filesystem::directory_entry TempSceneCode = SceneCode;
	TempSceneCode.replace_filename(TempSceneCode.path().stem().string() + ".temp" + TempSceneCode.path().extension().string());

	std::string SceneFileName = SceneCode.path().stem().string();

	SceneFileName[0] = toupper(SceneFileName[0]);

	for(u32 i = 1; i < SceneFileName.size(); i++)
	{
		if(SceneFileName[i-1] == '_')
		{
			SceneFileName[i] = toupper(SceneFileName[i]);
		}
	}
	SceneFileName.erase(std::remove(SceneFileName.begin(), SceneFileName.end(), '_'), SceneFileName.end());
	std::string CreateFunctionName = SceneFileName + "Create";

	std::ifstream SourceFile(SceneCode.path(), std::ios::binary);
    std::ofstream DestFile(TempSceneCode.path(), std::ios::binary);
    DestFile << SourceFile.rdbuf();
	DestFile.close();

	library_block SceneLibrary;
	game_scene_create* NewGameScene = (game_scene_create*)window::GetProcAddr(SceneLibrary, TempSceneCode.path().string().c_str(), CreateFunctionName.c_str());

	if(NewGameScene)
	{
		std::unique_ptr<scene> ScenePtr(NewGameScene());
		Scenes.push_back(std::move(ScenePtr));

		scene_info NewInfo = {};
		NewInfo.Module = SceneLibrary;
		NewInfo.Name = SceneFileName;
		NewInfo.Path = SceneCode.path().string();
		NewInfo.LastFileCreation = SceneCode.last_write_time();
		NewInfo.IsInitialized = true;
		Infos.push_back(NewInfo);
	}
}

void scene_manager::
LoadAllScenes(std::string ScenesPath)
{
	for(const std::filesystem::directory_entry& SceneCode : std::filesystem::directory_iterator(ScenesPath))
	{
#if _WIN32
		if(SceneCode.is_regular_file() && SceneCode.path().extension() == ".dll" && SceneCode.path().filename().string().find(".temp.dll") == std::string::npos) 
#else
		if(SceneCode.is_regular_file() && SceneCode.path().extension() == ".lscene" && SceneCode.path().filename().string().find(".temp.lscene") == std::string::npos) 
#endif
		{
			LoadScene(SceneCode);
        }
	}
}

// TODO: Think about something more efficient here if possible
void scene_manager::
UpdateScenes(std::string ScenesPath)
{
	u32 SceneIdx = 0;
	for(const auto& SceneCode : std::filesystem::directory_iterator(ScenesPath))
	{
#if _WIN32
		if(SceneCode.is_regular_file() && SceneCode.path().extension() == ".dll" && SceneCode.path().filename().string().find(".temp.dll") == std::string::npos) 
#else
		if(SceneCode.is_regular_file() && SceneCode.path().extension() == ".lscene" && SceneCode.path().filename().string().find(".temp.lscene") == std::string::npos) 
#endif
		{
            std::string SceneFileName = SceneCode.path().string();

			auto FoundIt = std::find_if(Infos.begin(), Infos.end(), [&SceneFileName](const scene_info& Info){return Info.Path == SceneFileName;});

			if (FoundIt != Infos.end()) 
			{
				SceneIdx = std::distance(Infos.begin(), FoundIt);
				if(Infos[SceneIdx].LastFileCreation != SceneCode.last_write_time())
				{
					Infos[SceneIdx].IsInitialized = false;
					std::filesystem::directory_entry TempSceneCode = SceneCode;
					TempSceneCode.replace_filename(TempSceneCode.path().stem().string() + ".temp" + TempSceneCode.path().extension().string());

					SceneFileName = SceneCode.path().stem().string();
					SceneFileName[0] = toupper(SceneFileName[0]);

					for(u32 i = 1; i < SceneFileName.size(); i++)
					{
						if(SceneFileName[i-1] == '_')
						{
							SceneFileName[i] = toupper(SceneFileName[i]);
						}
					}
					SceneFileName.erase(std::remove(SceneFileName.begin(), SceneFileName.end(), '_'), SceneFileName.end());
					std::string CreateFunctionName = SceneFileName + "Create";

					{
						std::unique_ptr<scene> OldScene(std::move(Scenes[SceneIdx]));
					}

					window::FreeLoadedLibrary(Infos[SceneIdx].Module);

					std::filesystem::remove(TempSceneCode.path());
					std::ifstream SourceFile(SceneCode.path(), std::ios::binary);
					std::ofstream DestFile(TempSceneCode.path(), std::ios::binary);
					DestFile << SourceFile.rdbuf();
					DestFile.close();

					game_scene_create* NewGameScene = (game_scene_create*)window::GetProcAddr(Infos[SceneIdx].Module, TempSceneCode.path().string().c_str(), CreateFunctionName.c_str());

					if(NewGameScene)
					{
						std::unique_ptr<scene> ScenePtr(NewGameScene());

						Scenes[SceneIdx] = std::move(ScenePtr);
						Infos[SceneIdx].LastFileCreation = SceneCode.last_write_time();
						Infos[SceneIdx].IsInitialized = true;
					}
				}
			} 
			else 
			{
				LoadScene(SceneCode);
			}
		}
	}
}

void scene_manager::
SaveSceneStateToFile(const char* FilePath)
{
}

void scene_manager::
LoadSceneStateFromFile(const char* FilePath)
{
}

void scene_manager::
StartScene(window& Window)
{
	if(!Scenes[CurrentScene]->IsInitialized)
	{
		GlobalGeometries.Clear();
		GlobalDebugGeometries.Clear();

		Scenes[CurrentScene]->Start();

		Scenes[CurrentScene]->Registry.AddSystem<light_sources_system>();
		Scenes[CurrentScene]->Registry.AddSystem<world_update_system>();
		Scenes[CurrentScene]->Registry.AddSystem<render_system>();
		Scenes[CurrentScene]->Registry.AddSystem<render_debug_system>();
		Scenes[CurrentScene]->Registry.AddSystem<ui_debug_system>();

		Scenes[CurrentScene]->Registry.UpdateSystems();

		Scenes[CurrentScene]->Registry.GetSystem<render_system>()->Setup(Window, MeshCompCullingCommonData);
		Scenes[CurrentScene]->Registry.GetSystem<render_debug_system>()->Setup(Window, MeshCompCullingCommonData);
	}
}

void scene_manager::
UpdateScene(window& Window, global_pipeline_context* PipelineContext, alloc_vector<light_source>& GlobalLightSources)
{
	if(Scenes[CurrentScene]->IsInitialized)
	{
		// TODO: Unload old scene, save scene state and load new scene. Load old scene saved state if old one is loaded respectively
		Scenes[CurrentScene]->Reset();
		Scenes[CurrentScene]->Update();

		Scenes[CurrentScene]->Registry.GetSystem<world_update_system>()->SubscribeToEvents(Window.EventsDispatcher);

		Scenes[CurrentScene]->Registry.GetSystem<light_sources_system>()->Update(WorldUpdate, GlobalLightSources);

		Scenes[CurrentScene]->Registry.GetSystem<render_system>()->UpdateResources(Window, GlobalLightSources, WorldUpdate, PipelineContext->BackBufferIndex);
		Scenes[CurrentScene]->Registry.GetSystem<render_debug_system>()->UpdateResources(Window, GlobalLightSources, WorldUpdate, PipelineContext->BackBufferIndex);
	}
}

void scene_manager::
RenderScene(window& Window, global_pipeline_context* PipelineContext,
			alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility, 
			alloc_vector<mesh_draw_command>& DynamicDebugInstances, alloc_vector<u32>& DynamicDebugVisibility,
			alloc_vector<light_source>& GlobalLightSources)
{
	if(Scenes[CurrentScene]->IsInitialized)
	{
		Scenes[CurrentScene]->Registry.GetSystem<world_update_system>()->Update(Window, WorldUpdate, MeshCompCullingCommonData, Scenes[CurrentScene]->GlobalLightPos);
		Scenes[CurrentScene]->Registry.GetSystem<render_system>()->Render(Window, PipelineContext, WorldUpdate, MeshCompCullingCommonData, DynamicMeshInstances, DynamicMeshVisibility, GlobalLightSources);
		Scenes[CurrentScene]->Registry.GetSystem<render_debug_system>()->Render(Window, PipelineContext, WorldUpdate, MeshCompCullingCommonData, DynamicDebugInstances, DynamicDebugVisibility);
	}
}

void scene_manager::
RenderUI()
{
	if(Scenes[CurrentScene]->IsInitialized)
	{
		Scenes[CurrentScene]->Registry.GetSystem<ui_debug_system>()->Update(WorldUpdate, MeshCompCullingCommonData);
	}
}
