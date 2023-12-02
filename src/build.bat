@echo off
if not defined DevEnvDir (
	call vcvarsall x64
)

setlocal EnableDelayedExpansion

set VulkanInc="%VULKAN_SDK%\Include"
set VulkanLib="%VULKAN_SDK%\Lib"

set CommonCompFlags=/std:c++latest /Zc:__cplusplus -fp:fast -nologo -MTd -EHsc -Od -Oi -WX- -W4 -GR -Gm- -GS -FC -Zi -D_MBCS -wd4005 -wd4100 -wd4127 -wd4189 -wd4201 -wd4238 -wd4244 -wd4267 -wd4324 -wd4505
set CommonLinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console /NODEFAULTLIB:MSVCRT

set PlatformCppFiles="..\src\main.cpp"
set GameCppFiles="..\src\game_main.cpp"

set DepthCascades=-DDEPTH_CASCADES_COUNT=3
set UseDebugColorBlend=-DDEBUG_COLOR_BLEND=0
set GBufferCount=-DGBUFFER_COUNT=5
set LightSourcesMax=-DLIGHT_SOURCES_MAX_COUNT=1024

if not exist ..\build\ mkdir ..\build\
if not exist ..\build\scenes\ mkdir ..\build\scenes\
if not exist ..\build\shaders\ mkdir ..\build\shaders\

for /f "tokens=1-3 delims=:" %%a in ("%time%") do (
	set /a "StartTime=%%a*3600 + %%b*60 + %%c"
)

rem goto shader_build_skip
glslangValidator ..\shaders\mesh.vert.glsl %DepthCascades% -o ..\build\shaders\mesh.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.frag.glsl -gVS -g %DepthCascades% %UseDebugColorBlend% -o ..\build\shaders\mesh.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.vert.glsl %DepthCascades% -o ..\build\shaders\mesh.dbg.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.frag.glsl %DepthCascades% -o ..\build\shaders\mesh.dbg.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.dbg.comp.glsl -gVS -g -o ..\build\shaders\mesh.dbg.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.sdw.vert.glsl %DepthCascades% -o ..\build\shaders\mesh.sdw.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.sdw.frag.glsl -o ..\build\shaders\mesh.sdw.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.pnt.sdw.vert.glsl %DepthCascades% -o ..\build\shaders\mesh.pnt.sdw.vert.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\mesh.pnt.sdw.frag.glsl -o ..\build\shaders\mesh.pnt.sdw.frag.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\color_pass.comp.glsl %GBufferCount% %LightSourcesMax% %UseDebugColorBlend% %DepthCascades% -gVS -g -o ..\build\shaders\color_pass.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\screen_space_ambient_occlusion.comp.glsl %GBufferCount% %DepthCascades% -gVS -g -o ..\build\shaders\screen_space_ambient_occlusion.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\indirect_cull_frust.comp.glsl -o ..\build\shaders\indirect_cull_frust.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\indirect_cull_occl.comp.glsl -o ..\build\shaders\indirect_cull_occl.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\depth_reduce.comp.glsl -o ..\build\shaders\depth_reduce.comp.spv -e main --target-env vulkan1.3
glslangValidator ..\shaders\blur.comp.glsl -o ..\build\shaders\blur.comp.spv -e main --target-env vulkan1.3
:shader_build_skip

pushd ..\build\scenes\
del *.pdb > NUL 2> NUL
for %%f in ("..\..\src\game_scenes\*.cpp") do (
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
	cl %CommonCompFlags% "!FileName!" /LD /Fe"!BaseName!" %DepthCascades% -DENGINE_EXPORT_CODE /link %CommonLinkFlags% /EXPORT:%ExportName% -PDB:ce_!BaseName!_%random%.pdb
)
popd

pushd ..\build\
del *.pdb > NUL 2> NUL
cl %CommonCompFlags% /I%VulkanInc% user32.lib kernel32.lib vulkan-1.lib %PlatformCppFiles% %UseDebugColorBlend% %DepthCascades% %GBufferCount% %LightSourcesMax% /Fe"Cynosure Engine" /link %CommonLinkFlags% /LIBPATH:%VulkanLib% -PDB:ce_%random%.pdb 
popd

for /f "tokens=1-3 delims=:" %%a in ("%time%") do (
	set /a "EndTime=%%a*3600 + %%b*60 + %%c"
)

set /a ElapsedTime=%EndTime%-%StartTime%

echo Compilation took %ElapsedTime% seconds.

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
