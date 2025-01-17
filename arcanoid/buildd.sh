#! /usr/bin/bash

CommonCompFlags="-std=c++2a -O0 -ffast-math -w -Wall -Wextra -Wno-error -g3 -pthread -MD -DCE_DEBUG -frtti -fPIC -march=native"

DepthCascades="-DDEPTH_CASCADES_COUNT=3"
UseDebugColorBlend="-DDEBUG_COLOR_BLEND=0"
GBufferCount="-DGBUFFER_COUNT=5"
LightSourcesMax="-DLIGHT_SOURCES_MAX_COUNT=256"
VoxelGridSize="-DVOXEL_SIZE=128"

GameModuleName="arcanoid"

mkdir -p ../build

pushd ../build
g++ $CommonCompFlags $DepthCascades $UseDebugColorBlend $GBufferCount $LightSourcesMax $VoxelGridSize -I"../src" -I"../src/core/vendor" "../$GameModuleName/${GameModuleName}_game_module.cpp" $CommonLinkFlags -shared -o "game_module.sce"
popd

exit 0
