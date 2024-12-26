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

if not exist ..\build\ mkdir ..\build\

pushd ..\build\
:: clang++ -fms-runtime-lib=dll ..\generate.cpp -o generate.exe -lversion -lmsvcrt -lclangAPINotes -lclangEdit -lclangBasic -lclangTooling -lclangFrontendTool -lclangCodeGen -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangASTMatchers -lclangSerialization -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangDriver -lclangFrontend -lclangParse -lclangAST -lclangLex -lclangSupport !llvm_flags! -Xlinker /NODEFAULTLIB:libcmt.lib
popd
..\build\generate.exe --extra-arg-before=-std=c++20 --extra-arg-before=-w --extra-arg-before=-Wall --extra-arg-before=-Wextra --extra-arg-before=-Wno-error --extra-arg-before=-Wno-narrowing --extra-arg-before=-DDEPTH_CASCADES_COUNT=3 --extra-arg-before=-DDEBUG_COLOR_BLEND=0 --extra-arg-before=-DGBUFFER_COUNT=5 --extra-arg-before=-DLIGHT_SOURCES_MAX_COUNT=256 --extra-arg-before=-DVOXEL_SIZE=128 --extra-arg-before=-I"..\src" --extra-arg-before=-I"..\src\core\vendor" .\core\platform\main.cpp

pushd ..\build\
:: cl -MD -O2 /LD ..\src\core\vendor\imgui\imgui*.cpp
cl %CommonCompFlags% /D_DLL ..\src\core\gfx\renderer.cpp /I%VulkanInc% /I"..\src" /I"..\src\core\vendor" /I"..\src\core\gfx\vendor" user32.lib kernel32.lib gdi32.lib shell32.lib d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib ..\libs\dxcompiler.lib vulkan-1.lib glslang.lib HLSL.lib OGLCompiler.lib OSDependent.lib MachineIndependent.lib SPIRV.lib SPIRV-Tools.lib SPIRV-Tools-opt.lib GenericCodeGen.lib glslang-default-resource-limits.lib SPVRemapper.lib spirv-cross-core.lib spirv-cross-cpp.lib spirv-cross-glsl.lib spirv-cross-hlsl.lib %UseDebugColorBlend% %DepthCascades% %GBufferCount% %LightSourcesMax% %VoxelGridSize% /LD /link %CommonLinkFlags% /OUT:"renderer.dll" /LIBPATH:%VulkanLib%

cl %CommonCompFlags% %UseDebugColorBlend% %DepthCascades% %GBufferCount% %LightSourcesMax% %VoxelGridSize% imgui.obj imgui_demo.obj imgui_draw.obj imgui_tables.obj imgui_widgets.obj renderer.lib ..\libs\freetype.lib user32.lib kernel32.lib gdi32.lib shell32.lib winmm.lib ..\src\core\platform\main.cpp ..\src\core\platform\win32\win32_main.cpp /I"..\src\core\vendor" /I"..\src" /Fe"Cynosure Engine" /link %CommonLinkFlags% /NODEFAULTLIB:libcmt
popd
