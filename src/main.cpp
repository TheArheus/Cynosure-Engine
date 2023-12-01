#include "intrinsics.h"
#include "main.h"
#include "core/entity_component_system/systems.hpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "core/scene_manager/scene_manager.cpp"
#include "core/mesh_loader/mesh.cpp"
#include "core/gfx/vulkan/renderer_vulkan.cpp"
#include "core/platform/win32/win32_window.cpp"

#include <random>


// TODO: Handle multiple buttons being pressed
// TODO: Proper light source shadows
//
// TODO: Handle dynamic entities that updates every frame
//
// TODO: Implement reflections using cube maps
// 
// TODO: Minecraft like world rendering?
//			Voxels?
//
// TODO: Application abstraction
// TODO: Mesh animation component
//
// TODO: Only one scene at a time
// TODO: Implement sound system with openal???
//			sound component???
//
// TODO: Use only one staging buffer instead of many (a lot of memory usage here, current solution is totally not optimal)
// TODO: Implement mesh shading pipeline in the future
// TODO: Implement ray  tracing pipeline in the future


int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{
	window Window("3D Renderer");
	Window.InitGraphics();
	scene_manager SceneManager(Window);

	u32 GlobalMemorySize = MiB(128);
	void* MemoryBlock = malloc(GlobalMemorySize);

	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	double TimeLast = window::GetTimestamp();
	double TimeElapsed = 0.0;
	double TimeEnd = 0.0;
	double AvgCpuTime = 0.0;
	while(Window.IsRunning())
	{
		mesh DebugGeometries;

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
		
		auto Result = Window.ProcessMessages();
		if(Result) return *Result;

		SceneManager.UpdateScenes();
		if(!SceneManager.IsCurrentSceneInitialized()) continue;

		SceneManager.StartScene(Window);
		DebugGeometries.Load(SceneManager.GlobalDebugGeometries);
		SceneManager.UpdateScene(Window, GlobalMeshInstances, GlobalMeshVisibility, DebugMeshInstances, DebugMeshVisibility, GlobalLightSources);

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
