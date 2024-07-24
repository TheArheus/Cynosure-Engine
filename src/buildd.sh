#! /usr/bin/bash

CommonCompFlags="-std=c++2a -O0 -ffast-math -w -Wall -Wextra -Wno-error -g3 -pthread -MD -DCE_DEBUG -frtti -fPIC -march=native"

PlatformCppFiles="../src/linux_main.cpp"

DepthCascades="-DDEPTH_CASCADES_COUNT=3"
UseDebugColorBlend="-DDEBUG_COLOR_BLEND=0"
GBufferCount="-DGBUFFER_COUNT=5"
LightSourcesMax="-DLIGHT_SOURCES_MAX_COUNT=256"
VoxelGridSize="-DVOXEL_SIZE=128"

mkdir -p ../build ../build/scenes

pushd ../build/scenes
for f in ../../src/game_scenes/*.cpp; do
    FileName="$f"
    BaseName=$(basename "${FileName%.cpp}")
    ExportName=""

    IFS='_' read -ra tokens <<< "$BaseName"
    for token in "${tokens[@]}"; do
        ExportName+=$(echo "$token" | sed 's/.*/\u&/')
    done

    g++ $CommonCompFlags -I../../src -I../../src/core/vendor "$FileName" -lassimp -shared -o "$BaseName.lscene" $DepthCascades -DENGINE_EXPORT_CODE
done
popd

pushd ../build
g++ $CommonCompFlags ../src/main.cpp $PlatformCppFiles -I../src -I../src/core/vendor -Wl,-rpath=../libs/ -L../libs/ -o "Cynosure Engine" -ldxcompiler -ldl -lglfw -lglslang -lvulkan -lMachineIndependent -lOSDependent -lGenericCodeGen -lOGLCompiler -lSPIRV -lSPIRV-Tools -lSPIRV-Tools-opt -lglslang-default-resource-limits -lSPVRemapper -lspirv-cross-core -lspirv-cross-cpp -lspirv-cross-glsl -lspirv-cross-hlsl -lassimp -lpthread $UseDebugColorBlend $DepthCascades $GBufferCount $LightSourcesMax $VoxelGridSize
popd

exit 0
