 
void engine::
Init(const std::vector<std::string>& args)
{
	Window.InitVulkanGraphics();
	//Window.InitDirectx12Graphics();

	CreateGuiContext(&Window);

	Registry.SetupSystems();

	AssetStore.AddFont("roboto", "../assets/fonts/OpenSans-Regular.ttf", MAX_FONT_SIZE);
	SetGuiFont(AssetStore.GetFont("roboto"), 18);
}

int engine::
Run()
{
	double FrameRateToChoose = 60.0;
    const double TargetFrameRate = 1000.0 / FrameRateToChoose;
    double TimeLast = window::GetTimestamp();

	double SmoothedFPS = FrameRateToChoose;
	const double SmoothingFactor = 0.05; // NOTE: 0 (slow) and 1 (fast)

	while(Window.IsRunning())
	{
		auto Result = window::ProcessMessages();
		if(Result) break;

		Window.NewFrame();
		Window.EmitEvents();

        double FrameTime = TargetFrameRate - (window::GetTimestamp() - TimeLast);
        if (FrameTime > 0 && FrameTime < TargetFrameRate)
        {
            window::SleepFor(TargetFrameRate - FrameTime);
        }
		double FrameDeltaTime = window::GetTimestamp() - TimeLast;
#ifdef CE_DEBUG
		if (FrameDeltaTime > TargetFrameRate) FrameDeltaTime = TargetFrameRate;
#endif
		TimeLast = window::GetTimestamp();

		UpdateModule();
		Module->Update(FrameDeltaTime);

		double InstantFPS = 1000.0 / FrameDeltaTime;
		SmoothedFPS = SmoothingFactor * InstantFPS + (1.0 - SmoothingFactor) * SmoothedFPS;
		std::string FrameSpeedString = "Frame: " + std::to_string(1000.0 / SmoothedFPS) + "ms, " + std::to_string(SmoothedFPS) + "fps";
		GuiLabel(FrameSpeedString, vec2(0, Window.Height));

		if(!Window.IsGfxPaused)
		{
			Window.Gfx.Compile();
			Window.Gfx.Execute();
		}

		Window.Gfx.SwapBuffers();
		Window.UpdateStates();
		Allocator.UpdateAndReset();
	}

	window::EventsDispatcher.Reset();
	Registry.ClearAll();

	Module.reset();
	window::FreeLoadedLibrary(ModuleInfo.Module);

	{
		std::filesystem::path TempPath("../build/game_module.sce");
		{
			auto Stem = TempPath.stem().string();
			auto Ext  = TempPath.extension().string();
			TempPath.replace_filename(Stem + ".temp" + Ext);
		}

		if(std::filesystem::exists(TempPath))
		{
			std::filesystem::remove(TempPath);
		}
	}

	DestroyGuiContext();
	return 0;
}

void engine::
LoadModule()
{
	window::EventsDispatcher.Reset();
	Registry.ClearAll();

    std::filesystem::path OriginalPath("../build/game_module.sce");
    if (!std::filesystem::exists(OriginalPath) || !std::filesystem::is_regular_file(OriginalPath))
    {
        std::cerr << "Module file does not exist or is not a file: " 
                  << "../build/game_module.sce" << std::endl;
        return;
    }

    std::filesystem::path TempPath = OriginalPath;
    {
        auto Stem = TempPath.stem().string();
        auto Ext  = TempPath.extension().string();
        TempPath.replace_filename(Stem + ".temp" + Ext);
    }

    if (ModuleInfo.IsInitialized)
    {
        Module.reset();
        window::FreeLoadedLibrary(ModuleInfo.Module);
        ModuleInfo.IsInitialized = false;
    }

    if(std::filesystem::exists(TempPath))
    {
        std::filesystem::remove(TempPath);
    }

    {
        std::ifstream SourceFile(OriginalPath, std::ios::binary);
        std::ofstream DestFile(TempPath, std::ios::binary);
        DestFile << SourceFile.rdbuf();
    }

    std::string CreateFunctionName = "GameModuleCreate";
    game_module_create* GameModuleCreate = (game_module_create*)
		                                    window::GetProcAddr(ModuleInfo.Module, 
			                                TempPath.string().c_str(), 
											CreateFunctionName.c_str());

    if (!GameModuleCreate)
    {
        return;
    }

    Module.reset(GameModuleCreate(Window, window::EventsDispatcher, Registry, Window.Gfx, GlobalGuiContext));

    ModuleInfo.Name             = OriginalPath.stem().string();
    ModuleInfo.Path             = OriginalPath.string();
    ModuleInfo.LastFileCreation = std::filesystem::last_write_time(OriginalPath);
    ModuleInfo.IsInitialized    = (Module != nullptr);

    if (Module && Module->IsInitialized)
    {
        Module->ModuleStart();
    }
}

void engine::
UpdateModule()
{
    std::filesystem::path OriginalPath("../build/game_module.sce");

    auto CurrentFileTime = std::filesystem::last_write_time(OriginalPath);
    if (CurrentFileTime != ModuleInfo.LastFileCreation && !window::IsFileLocked(OriginalPath))
    {
        LoadModule();
    }
}
