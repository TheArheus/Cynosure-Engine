#include "intrinsics.h"
#include "main.h"
#include "core/entity_component_system/systems.hpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "core/scene_manager/scene_manager.cpp"
#include "core/mesh_loader/mesh.cpp"
#include "core/gfx/renderer.cpp"
#include <D3D12MemAlloc.cpp>

#include <random>


// TODO: Fix debug rendering for dx12
// TODO: Handle dynamic entities that updates every frame
//			One entity - one instance for a group of the object(would be easy to handle each instance and add to them the component if needed)
//
// TODO: Implement reflections using cube maps
//
// TODO: Application abstraction
// TODO: Mesh animation component
//
// TODO: Only one scene at a time
// TODO: Implement sound system with openal???
//
// TODO: Better memory allocation. Reallocation of the GlobalMemoryBlock when needed?
// TODO: Use only one staging buffer instead of many (a lot of memory usage here, current solution is totally not optimal)
// TODO: Implement mesh shading pipeline in the future
// TODO: Implement ray  tracing pipeline in the future



int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{
	window Window(1280, 720, "3D Renderer");
	//Window.InitVulkanGraphics();
	Window.InitDirectx12Graphics();
	scene_manager SceneManager(Window);
	global_pipeline_context* PipelineContext = Window.Gfx.CreateGlobalPipelineContext();

	u32 GlobalMemorySize = MiB(128);
	void* MemoryBlock = malloc(GlobalMemorySize);

	double TargetFrameRate = 1.0 / 60.0 * 1000.0; // Frames Per Milliseconds

	double TimeLast = window::GetTimestamp();
	double TimeElapsed = 0.0;
	double TimeEnd = 0.0;
	double AvgCpuTime = 0.0;
	while(Window.IsRunning())
	{
		linear_allocator SystemsAllocator(GlobalMemorySize, MemoryBlock);
		linear_allocator LightSourcesAlloc(sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT, SystemsAllocator.Allocate(sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT));
		linear_allocator GlobalMeshInstancesAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator GlobalMeshVisibleAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator DebugMeshInstancesAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator DebugMeshVisibleAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));

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
			PipelineContext->Begin(Window.Gfx.Backend);

			SceneManager.RenderScene(Window, PipelineContext, GlobalMeshInstances, GlobalMeshVisibility, DebugMeshInstances, DebugMeshVisibility, GlobalLightSources);

			PipelineContext->DebugGuiBegin(Window.Gfx.Backend, Window.Gfx.GfxColorTarget);
			SceneManager.RenderUI();
			PipelineContext->DebugGuiEnd(Window.Gfx.Backend);

			PipelineContext->EmplaceColorTarget(Window.Gfx.Backend, Window.Gfx.GfxColorTarget);
			PipelineContext->Present(Window.Gfx.Backend);
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

		std::string Title = "Frame " + std::to_string(AvgCpuTime) + "ms, " + std::to_string(1.0 / AvgCpuTime * 1000.0) + "fps";
		Window.SetTitle(Title);
	}

	free(MemoryBlock);

	return 0;
}

int main(int argc, char* argv[])
{
	std::string CommandLine;
	for(int i = 1; i < argc; ++i)
	{
		CommandLine += std::string(argv[i]);
		if(i != (argc - 1))
		{
			CommandLine += " ";
		}
	}

	return WinMain(0, 0, const_cast<char*>(CommandLine.c_str()), SW_SHOWNORMAL);
}
