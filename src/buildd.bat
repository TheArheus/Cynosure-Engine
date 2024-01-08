@echo off
if not defined DevEnvDir (
	call vcvarsall x64
)

setlocal EnableDelayedExpansion

set VulkanInc="%VULKAN_SDK%\Include"
set VulkanLib="%VULKAN_SDK%\Lib"

set CommonCompFlags=/std:c++latest /Zc:__cplusplus -fp:fast -nologo -MDd -EHsc -Od -Oi -WX- -W4 -GR -Gm- -GS -FC -Zi -D_MBCS -wd4005 -wd4100 -wd4127 -wd4189 -wd4201 -wd4238 -wd4244 -wd4267 -wd4324 -wd4715
set CommonLinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console /ignore:4099

set PlatformCppFiles="..\src\main.cpp"
set GameCppFiles="..\src\game_main.cpp"

set DepthCascades=-DDEPTH_CASCADES_COUNT=3
set UseDebugColorBlend=-DDEBUG_COLOR_BLEND=0
set GBufferCount=-DGBUFFER_COUNT=5
set LightSourcesMax=-DLIGHT_SOURCES_MAX_COUNT=256

if not exist ..\build\ mkdir ..\build\
if not exist ..\build\scenes\ mkdir ..\build\scenes\

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
	cl %CommonCompFlags% /I"..\..\src" "!FileName!" /LD /Fe"!BaseName!" %DepthCascades% -DENGINE_EXPORT_CODE /link %CommonLinkFlags% /EXPORT:%ExportName% -PDB:ce_!BaseName!_%random%.pdb
)
popd

pushd ..\build\
del *.pdb > NUL 2> NUL
cl %CommonCompFlags% /I%VulkanInc% /I"..\src" user32.lib kernel32.lib gdi32.lib shell32.lib d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib dxcompiler.lib vulkan-1.lib glslangd.lib HLSLd.lib OGLCompilerd.lib OSDependentd.lib MachineIndependentd.lib SPIRVd.lib SPIRV-Toolsd.lib SPIRV-Tools-optd.lib GenericCodeGend.lib glslang-default-resource-limitsd.lib SPVRemapperd.lib spirv-cross-cored.lib spirv-cross-cppd.lib spirv-cross-glsld.lib spirv-cross-hlsld.lib %PlatformCppFiles% %UseDebugColorBlend% %DepthCascades% %GBufferCount% %LightSourcesMax% /Fe"Cynosure Engine" /link %CommonLinkFlags% /LIBPATH:"..\libs\" /LIBPATH:%VulkanLib% -PDB:ce_%random%.pdb 
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
