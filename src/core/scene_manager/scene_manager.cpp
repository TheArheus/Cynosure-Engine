
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

	CopyFile(SceneCode.path().string().c_str(), TempSceneCode.path().string().c_str(), FALSE);

	HMODULE SceneLibrary;
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
		if(SceneCode.is_regular_file() && SceneCode.path().extension() == ".dll" && SceneCode.path().filename().string().find(".temp.dll") == std::string::npos) 
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
		if(SceneCode.is_regular_file() && SceneCode.path().extension() == ".dll" && SceneCode.path().filename().string().find(".temp.dll") == std::string::npos) 
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
					DeleteFileA(TempSceneCode.path().string().c_str());
					CopyFile(SceneCode.path().string().c_str(), TempSceneCode.path().string().c_str(), FALSE);

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

		Scenes[CurrentScene]->Registry.UpdateSystems();

		Scenes[CurrentScene]->Registry.GetSystem<render_system>()->Setup(Window, GlobalHeap, MeshCompCullingCommonData);
		Scenes[CurrentScene]->Registry.GetSystem<render_debug_system>()->Setup(Window, GlobalHeap, MeshCompCullingCommonData, GfxColorTarget.Info.Format);
	}
}

void scene_manager::
UpdateScene(window& Window, 
			alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility, 
			alloc_vector<mesh_draw_command>& DynamicDebugInstances, alloc_vector<u32>& DynamicDebugVisibility,
			alloc_vector<light_source>& GlobalLightSources)
{
	if(Scenes[CurrentScene]->IsInitialized)
	{
		Scenes[CurrentScene]->Reset();
		Scenes[CurrentScene]->Update();

		Scenes[CurrentScene]->Registry.GetSystem<world_update_system>()->SubscribeToEvents(Window.EventsDispatcher);

		if(!Window.IsGfxPaused)
		{
			Scenes[CurrentScene]->Registry.GetSystem<light_sources_system>()->Update(WorldUpdate, GlobalLightSources);
			Scenes[CurrentScene]->Registry.GetSystem<render_system>()->UpdateResources(Window, GlobalLightSources, WorldUpdate);

			PipelineContext.Begin(Window.Gfx);

			Scenes[CurrentScene]->Registry.GetSystem<world_update_system>()->Update(Window, WorldUpdate, MeshCompCullingCommonData, Scenes[CurrentScene]->GlobalLightPos);
			Scenes[CurrentScene]->Registry.GetSystem<render_system>()->Render(Window, PipelineContext, GfxColorTarget, GfxDepthTarget, DebugCameraViewDepthTarget, WorldUpdate, MeshCompCullingCommonData, DynamicMeshInstances, DynamicMeshVisibility, GlobalLightSources);
			Scenes[CurrentScene]->Registry.GetSystem<render_debug_system>()->Render(Window, PipelineContext, GfxColorTarget, GfxDepthTarget, WorldUpdate, MeshCompCullingCommonData, DynamicDebugInstances, DynamicDebugVisibility);

			PipelineContext.EmplaceColorTarget(Window.Gfx, GfxColorTarget);
			PipelineContext.Present(Window.Gfx);
		}
	}
}
