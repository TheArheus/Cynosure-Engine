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

set DepthCascades=-DDEPTH_CASCADES_COUNT=3
set UseDebugColorBlend=-DDEBUG_COLOR_BLEND=0
set GBUFFER_COUNT=-DGBUFFER_COUNT=5

glslangValidator ..\shaders\mesh.vert.glsl %DepthCascades% -gVS -g -o ..\build\mesh.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.frag.glsl %DepthCascades% %UseDebugColorBlend% -gVS -g -o ..\build\mesh.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.vert.glsl %DepthCascades% -o ..\build\mesh.dbg.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.frag.glsl %DepthCascades% -o ..\build\mesh.dbg.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.sdw.vert.glsl %DepthCascades% -o ..\build\mesh.sdw.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.sdw.frag.glsl -o ..\build\mesh.sdw.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\color_pass.comp.glsl %UseDebugColorBlend% %DepthCascades% -gVS -g -o ..\build\color_pass.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\screen_space_ambient_occlusion.comp.glsl %DepthCascades% -gVS -g -o ..\build\screen_space_ambient_occlusion.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\indirect_cull_frust.comp.glsl -o ..\build\indirect_cull_frust.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\indirect_cull_occl.comp.glsl -o ..\build\indirect_cull_occl.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\depth_reduce.comp.glsl -o ..\build\depth_reduce.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\blur.comp.glsl -o ..\build\blur.comp.spv -e main --target-env vulkan1.3

if not exist ..\build\ mkdir ..\build\
pushd ..\build\

for %%f in ("..\src\game_scenes\*.cpp") do (
	set FileName=%%f
	set BaseName=%%~nf
    set ExportName=
    
	for /F "tokens=1,* delims=_" %%a in ("!BaseName!") do (
        call :ProcessToken %%a
        set "RestOfName=%%b"
    )

    :NextToken
    if not "!RestOfName!"=="" (
        for /F "tokens=1,* delims=_" %%a in ("!RestOfName!") do (
            call :ProcessToken %%a
            set "RestOfName=%%b"
        )
        goto NextToken
    )
    
    set ExportName=!ExportName!Create
	cl %CommonCompFlags% "!FileName!" /LD /Fe"!BaseName!" /Fd"!BaseName!" -DENGINE_EXPORT_CODE /link %CommonLinkFlags% /EXPORT:%ExportName%
)
cl %CommonCompFlags% /I%VulkanInc% user32.lib kernel32.lib vulkan-1.lib %PlatformCppFiles% %UseDebugColorBlend% %DepthCascades% %GBUFFER_COUNT% /Fe"Cynosure Engine" /Fd"Cynosure Engine" /link %CommonLinkFlags% /LIBPATH:%VulkanLib%

popd

goto :eof

:ProcessToken
	call :CapitalizeFirstLetter %1
	set "ExportName=!ExportName!!CapWord!"

goto :eof

:CapitalizeFirstLetter
    set InputWord=%1
    set FirstChar=%InputWord:~0,1%
    set RemainingChars=%InputWord:~1%
    for %%i in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
        if /i "!FirstChar!"=="%%i" set FirstChar=%%i
    )
    set CapWord=!FirstChar!!RemainingChars!
    exit /b
