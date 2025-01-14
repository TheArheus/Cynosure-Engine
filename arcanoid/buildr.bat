@echo off
if not defined DevEnvDir (
	call vcvarsall x64
)

setlocal EnableDelayedExpansion

set VulkanInc="%VULKAN_SDK%\Include"
set VulkanLib="%VULKAN_SDK%\Lib"

set CommonCompFlags=/std:c++latest /Zc:__cplusplus -fp:fast -nologo -MD -EHsc -O2 -Oi -WX- -W4 -GR -Gm- -GS -FC -Zi -D_MBCS -wd4005 -wd4100 -wd4127 -wd4189 -wd4201 -wd4238 -wd4244 -wd4267 -wd4315 -wd4324 -wd4505 -wd4715
set CommonLinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console /ignore:4099

set DepthCascades=-DDEPTH_CASCADES_COUNT=3
set UseDebugColorBlend=-DDEBUG_COLOR_BLEND=0
set GBufferCount=-DGBUFFER_COUNT=5
set LightSourcesMax=-DLIGHT_SOURCES_MAX_COUNT=256
set VoxelGridSize=-DVOXEL_SIZE=128

set GameModuleName=arcanoid

if not exist ..\build\ mkdir ..\build\
if exist ..\build\game_module_*.pdb del /F /Q ..\build\game_module_*.pdb

pushd ..\build\
cl %CommonCompFlags% %UseDebugColorBlend% %DepthCascades% %GBufferCount% %LightSourcesMax% %VoxelGridSize% imgui.obj imgui_demo.obj imgui_draw.obj imgui_tables.obj imgui_widgets.obj renderer.lib ..\libs\freetype.lib user32.lib kernel32.lib gdi32.lib shell32.lib winmm.lib /I"..\src" /I"..\src\core\vendor" "..\!GameModuleName!\!GameModuleName!_game_module.cpp" ..\libs\assimp-vc143-mt.lib /LD /link %CommonLinkFlags% /EXPORT:GameModuleCreate /NODEFAULTLIB:LIBCMT /LIBPATH:"..\..\libs" /OUT:"game_module.sce" /PDB:game_module_%random%.pdb
popd
