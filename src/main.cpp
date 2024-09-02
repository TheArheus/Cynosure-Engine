#include "intrinsics.h"
#include "main.h"
#include "core/entity_component_system/systems.hpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "core/scene_manager/scene_manager.cpp"
#include "core/mesh_loader/mesh.cpp"
#include "core/gfx/renderer.cpp"

#if _WIN32
#include <D3D12MemAlloc.cpp>
#endif

#include <random>
#include <string_view>


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
// TODO: Implement mesh shading pipeline in the future
// TODO: Implement ray  tracing pipeline in the future



// TODO: make a class from this
[[nodiscard]] int engine_main([[maybe_unused]] const std::vector<std::string>& args)
{
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
		// TODO: Hide allocator creations
		linear_allocator LightSourcesAlloc(PushAllocArrayArg(light_source, LIGHT_SOURCES_MAX_COUNT));
		linear_allocator GlobalMeshInstancesAlloc(PushAllocSizeArg(MiB(16)));
		linear_allocator GlobalMeshVisibleAlloc(PushAllocSizeArg(MiB(16)));
		linear_allocator DebugMeshInstancesAlloc(PushAllocSizeArg(MiB(16)));
		linear_allocator DebugMeshVisibleAlloc(PushAllocSizeArg(MiB(16)));

		alloc_vector<mesh_draw_command> GlobalMeshInstances(GlobalMeshInstancesAlloc);
		alloc_vector<u32> GlobalMeshVisibility(GlobalMeshVisibleAlloc);
		alloc_vector<mesh_draw_command> DebugMeshInstances(DebugMeshInstancesAlloc);
		alloc_vector<u32> DebugMeshVisibility(DebugMeshVisibleAlloc);
		alloc_vector<light_source> GlobalLightSources(LightSourcesAlloc);

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
