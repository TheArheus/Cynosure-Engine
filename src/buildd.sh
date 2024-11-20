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
clang++ ../generate.cpp -o generate $(llvm-config --cxxflags) -Wl,--start-group $(llvm-config --ldflags --system-libs --libs all) -lclangAPINotes -lclangEdit -lclangBasic -lclangTooling -lclangFrontendTool -lclangCodeGen -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangASTMatchers -lclangSerialization -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangDriver -lclangFrontend -lclangParse -lclangAST -lclangLex -lclangSupport -lclangIndex -lclang -Wl,--end-group
popd
../generate --extra-arg-before=-resource-dir=/home/deck/llvm-build/lib/clang/20 --extra-arg-before=-march=native --extra-arg-before=-std=c++20 --extra-arg-before=-w --extra-arg-before=-Wall --extra-arg-before=-Wextra --extra-arg-before=-Wno-error --extra-arg-before=-Wno-narrowing --extra-arg-before=-DDEPTH_CASCADES_COUNT=3 --extra-arg-before=-DDEBUG_COLOR_BLEND=0 --extra-arg-before=-DGBUFFER_COUNT=5 --extra-arg-before=-DLIGHT_SOURCES_MAX_COUNT=256 --extra-arg-before=-DVOXEL_SIZE=128 --extra-arg-before=-I/usr/include/vulkan --extra-arg-before=-I../src --extra-arg-before=-I../src/core/vendor ../src/main.cpp

pushd ../build
g++ $CommonCompFlags ../src/main.cpp $PlatformCppFiles -I../src -I../src/core/vendor -Wl,-rpath=../libs/ -L../libs/ -o "Cynosure Engine" -ldxcompiler -ldl -lglfw -lglslang -lvulkan -lMachineIndependent -lOSDependent -lGenericCodeGen -lOGLCompiler -lSPIRV -lSPIRV-Tools -lSPIRV-Tools-opt -lglslang-default-resource-limits -lSPVRemapper -lspirv-cross-core -lspirv-cross-cpp -lspirv-cross-glsl -lspirv-cross-hlsl -lassimp -lpthread $UseDebugColorBlend $DepthCascades $GBufferCount $LightSourcesMax $VoxelGridSize
popd

exit 0
