@echo off
if not defined DevEnvDir (
	call vcvarsall x64
)

setlocal EnableDelayedExpansion

set VulkanInc="%VULKAN_SDK%\Include"
set VulkanLib="%VULKAN_SDK%\Lib"

set CommonCompFlags=/std:c++latest /Zc:__cplusplus -fp:fast -nologo -MTd -EHsc -Od -Oi -WX- -W4 -GR- -Gm- -GS -FC -Zi -D_MBCS -wd4005 -wd4100 -wd4189 -wd4201 -wd4238 -wd4244 -wd4267 -wd4324
set CommonLinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console

set PlatformCppFiles="..\src\main.cpp"
set GameCppFiles="..\src\game_main.cpp"
rem set CppFiles= 
rem for /R %%f in (*.cpp) do (
rem 	set CppFiles=!CppFiles! "%%f"
rem )

set DepthCascades=-DDEPTH_CASCADES_COUNT=3
set UseDebugColorBlend=-DDEBUG_COLOR_BLEND=0

glslangValidator ..\shaders\mesh.vert.glsl %DepthCascades% -o ..\build\mesh.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.frag.glsl %UseDebugColorBlend% %DepthCascades% -gVS -g -o ..\build\mesh.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.vert.glsl %DepthCascades% -o ..\build\mesh.dbg.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.frag.glsl %DepthCascades% -o ..\build\mesh.dbg.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.sdw.vert.glsl %DepthCascades% -o ..\build\mesh.sdw.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.sdw.frag.glsl -o ..\build\mesh.sdw.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\indirect_cull_frust.comp.glsl -gVS -g -o ..\build\indirect_cull_frust.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\indirect_cull_occl.comp.glsl -gVS -g -o ..\build\indirect_cull_occl.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\depth_reduce.comp.glsl -o ..\build\depth_reduce.comp.spv -e main --target-env vulkan1.3

if not exist ..\build\ mkdir ..\build\
pushd ..\build\
cl %CommonCompFlags% %GameCppFiles% %DepthCascades% /Fe"game_code" /Fd"game_code" -DENGINE_EXPORT_CODE -LD /link %CommonLinkFlags% /EXPORT:GameUpdateAndRender /EXPORT:GameSetup
cl %CommonCompFlags% /I%VulkanInc% user32.lib kernel32.lib vulkan-1.lib %PlatformCppFiles% %UseDebugColorBlend% %DepthCascades% /Fe"Cynosure Engine" /Fd"Cynosure Engine" /link %CommonLinkFlags% /LIBPATH:%VulkanLib%
popd
