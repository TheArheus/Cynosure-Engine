@echo off
setlocal EnableDelayedExpansion

pushd ..\shaders\
for %%f in ("*.glsl") do (
	set FileName=%%f
	set BaseName=%%~nf
	set ShaderGlslStage=!BaseName:~-4,4!
	set ShaderStage=""

    if "!ShaderGlslStage!" == "vert" ( 
		set ShaderStage="vs_6_6" 
	)
    if "!ShaderGlslStage!" == "frag" ( 
		set ShaderStage="ps_6_6" 
	)
    if "!ShaderGlslStage!" == "comp" ( 
		set ShaderStage="cs_6_6" 
	)

	glslangValidator !FileName! -o !BaseName!.spv -DDEBUG_COLOR_BLEND=0 -DDEPTH_CASCADES_COUNT=3 -DGBUFFER_COUNT=5 -DLIGHT_SOURCES_MAX_COUNT=256 -e main --target-env vulkan1.3
	spirv-cross !BaseName!.spv --shader-model 66 --hlsl --output !BaseName!.hlsl
	del !BaseName!.spv
	dxc.exe !BaseName!.hlsl -O3 -E main -T !ShaderStage! -Fo !BaseName!.cso -Qembed_debug -Zi -Fd !BaseName!.pdb
	del !BaseName!.hlsl
	del !BaseName!.cso
	del !BaseName!.pdb
)
popd
