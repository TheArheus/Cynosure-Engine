#include "intrinsics.h"
#include "main.h"
#include "core/entity_component_system/systems.hpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "core/scene_manager/scene_manager.cpp"
#include "core/mesh_loader/mesh.cpp"
#include "core/gfx/renderer.cpp"

#if _WIN32
#include <D3D12MemAlloc.cpp>
#ifdef CE_DEBUG
#include <crtdbg.h>
#endif
#endif


// TODO: Implement render graph resource management
// there are several resource types:
//		Buffer:
//			1) The buffer initialized with data type(only for the upload?) // data type
//			2) The buffer initialized with array data type // buffer reference
//			3) Empty buffer // buffer reference
//		Texture:
//			1) Without loading the initial data // texture reference
//			2) And initialized with some kind of a texture // texture reference
//		Think about resource life time:
//			I guess, textures could live all the time. 
//			Buffers could live all the time only if they were empty created. If they were created via data type, then they could live only one game cycle?
//		Basically, there will be 2 types of lifecycle resources: transient(per render-pass or per-frame) and persistent(throughout of the program)
//		Garbage Collector-like system for those resources(transient to be destroyed if several frames wasn't used and persistent to be destroyed at the end of the program)

// TODO: Sky
// TODO: Image Based Lighting
// TODO: Atmospheric scattering
// TODO: Better shadows and shadow maps
// TODO: Ambient Occlusion fix or a better one
// TODO: Volumetric clouds
//
// TODO: Handle dynamic entities that updates every frame
//			One entity - one instance for a group of the object(would be easy to handle each instance and add to them the component if needed)
//
// TODO: Implement sound system with openal???
//
// TODO: Custom containers
//
// TODO: Implement mesh shading pipeline in the future
// TODO: Implement ray  tracing pipeline in the future



[[nodiscard]] int engine_main([[maybe_unused]] const std::vector<std::string>& args)
{
#ifdef CE_DEBUG
#if _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#endif
	window Window(1280, 720, "3D Renderer");
	//Window.InitVulkanGraphics();
	Window.InitDirectx12Graphics();
	scene_manager SceneManager;

	double TargetFrameRate = 1.0 / 60.0 * 1000.0; // Frames Per Milliseconds

	double TimeLast = window::GetTimestamp();
	double TimeElapsed = 0.0;
	double TimeEnd = 0.0;
	double AvgCpuTime = 0.0;
	u64 FrameIdx = 0;
	while(Window.IsRunning())
	{
		alloc_vector<mesh_draw_command> GlobalMeshInstances(16384);
		alloc_vector<u32> GlobalMeshVisibility(16384);
		alloc_vector<mesh_draw_command> DebugMeshInstances(16384);
		alloc_vector<u32> DebugMeshVisibility(16384);
		alloc_vector<light_source> GlobalLightSources(LIGHT_SOURCES_MAX_COUNT);

		Window.NewFrame();
		Window.EventsDispatcher.Reset();

		SceneManager.UpdateScenes();
		if(!SceneManager.IsCurrentSceneInitialized()) continue;

		SceneManager.StartScene(Window);
		SceneManager.UpdateScene(Window, GlobalLightSources);

		auto Result = Window.ProcessMessages();
		if(Result) return *Result;

		if(!Window.IsGfxPaused)
		{
			SceneManager.RenderScene(Window, GlobalMeshInstances, GlobalMeshVisibility, DebugMeshInstances, DebugMeshVisibility, GlobalLightSources);

			Window.Gfx.Compile();
			Window.Gfx.Execute(SceneManager);
		}

		Window.EmitEvents();

		Window.EventsDispatcher.DispatchEvents();

		TimeEnd = window::GetTimestamp();
		TimeElapsed = (TimeEnd - TimeLast);

#if 0
		if(TimeElapsed < TargetFrameRate)
		{
			while(TimeElapsed < TargetFrameRate)
			{
				double TimeToSleep = TargetFrameRate - TimeElapsed;
				if (TimeToSleep > 0) 
				{
					Sleep(static_cast<DWORD>(TimeToSleep));
				}
				TimeEnd = window::GetTimestamp();
				TimeElapsed = TimeEnd - TimeLast;
			}
		}
#endif

		TimeLast = TimeEnd;
		AvgCpuTime = 0.75 * AvgCpuTime + TimeElapsed * 0.25;

		// TODO: move this into text rendering
		std::string Title = "Frame " + std::to_string(AvgCpuTime) + "ms, " + std::to_string(1.0 / AvgCpuTime * 1000.0) + "fps";
		Window.SetTitle(Title);
		Allocator.UpdateAndReset();
	}

	return 0;
}
