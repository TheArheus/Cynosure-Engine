
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

	u32 CurrentScene = 0;
	std::vector<std::unique_ptr<scene>> Scenes;
	std::vector<scene_info> Infos;

	mesh GlobalGeometries;
	mesh GlobalDebugGeometries;

	bool DebugMeshesDrawEnabled = true;

	scene_manager(std::string ScenesPath = "..\\build\\scenes\\")
	{
		LoadAllScenes(ScenesPath);
	}

	// TODO: Load a scenes by their file names. Load and unload if scene was recompiled(many if needed, but for this I need to check this every times in the loop I guess but that is a lot of work and think about something better)
	void LoadScene(const std::filesystem::directory_entry& SceneCode);
	void LoadAllScenes(std::string ScenesPath = "..\\build\\scenes\\");
	void UpdateScenes(std::string ScenesPath = "..\\build\\scenes\\");

	bool IsCurrentSceneInitialized(){return Infos[CurrentScene].IsInitialized;}

	void StartScene();
	void UpdateScene(std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>>& GlobalInstances, 
					 std::vector<u32, allocator_adapter<u32, linear_allocator>>& GlobalMeshVisibility, 
					 std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>>& GlobalDebugInstances, 
					 std::vector<u32, allocator_adapter<u32, linear_allocator>>& DebugMeshVisibility,
					 std::vector<light_source, allocator_adapter<light_source, linear_allocator>>& GlobalLightSources);

	void SaveSceneStateToFile(const char* FilePath);
	void LoadSceneStateFromFile(const char* FilePath);
};

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
StartScene()
{
	if(!Scenes[CurrentScene]->IsInitialized)
	{
		GlobalGeometries.Clear();
		GlobalDebugGeometries.Clear();

		Scenes[CurrentScene]->Start();

		for(std::unique_ptr<object_behavior>& Object : Scenes[CurrentScene]->Objects)
		{
			GlobalGeometries.Load(Object->Mesh);
		}
		GlobalDebugGeometries.LoadDebug(GlobalGeometries);
	}
}

void scene_manager::
UpdateScene(std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>>& GlobalMeshInstances, 
			std::vector<u32, allocator_adapter<u32, linear_allocator>>& GlobalMeshVisibility, 
			std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>>& GlobalDebugInstances, 
			std::vector<u32, allocator_adapter<u32, linear_allocator>>& DebugMeshVisibility,
			std::vector<light_source, allocator_adapter<light_source, linear_allocator>>& GlobalLightSources)
{
	if(Scenes[CurrentScene]->IsInitialized)
	{
		Scenes[CurrentScene]->Reset();
		Scenes[CurrentScene]->Update();
		for(std::unique_ptr<object_behavior>& Object : Scenes[CurrentScene]->Objects)
		{
			GlobalMeshInstances.insert(GlobalMeshInstances.end(), Object->Instances.begin(), Object->Instances.end());
			GlobalMeshVisibility.insert(GlobalMeshVisibility.end(), Object->InstancesVisibility.begin(), Object->InstancesVisibility.end());
		}
		if(DebugMeshesDrawEnabled)
		{
			for(const mesh_draw_command_input& ObjectInstanceCommand : GlobalMeshInstances)
			{
				GlobalDebugInstances.push_back({{vec4(vec3(1, 0, 0), 1), 0, 0, 0, 0}, ObjectInstanceCommand.Translate, ObjectInstanceCommand.Scale, (ObjectInstanceCommand.MeshIndex-1)*3 + 1});
				DebugMeshVisibility.push_back(true);
			}
			for(const mesh_draw_command_input& ObjectInstanceCommand : GlobalMeshInstances)
			{
				GlobalDebugInstances.push_back({{vec4(vec3(0, 1, 0), 1), 0, 0, 0, 0}, ObjectInstanceCommand.Translate, ObjectInstanceCommand.Scale, (ObjectInstanceCommand.MeshIndex-1)*3 + 2});
				DebugMeshVisibility.push_back(true);
			}
			for(const mesh_draw_command_input& ObjectInstanceCommand : GlobalMeshInstances)
			{
				GlobalDebugInstances.push_back({{vec4(vec3(0, 0, 1), 1), 0, 0, 0, 0}, ObjectInstanceCommand.Translate, ObjectInstanceCommand.Scale, (ObjectInstanceCommand.MeshIndex-1)*3 + 3});
				DebugMeshVisibility.push_back(true);
			}
		}
		GlobalLightSources.insert(GlobalLightSources.end(), Scenes[CurrentScene]->LightSources.begin(), Scenes[CurrentScene]->LightSources.end());
	}
}
