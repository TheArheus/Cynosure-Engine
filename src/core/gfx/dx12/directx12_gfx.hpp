#pragma once

#include "directx/d3d12shader.h"
#include "directx/d3dx12.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

using namespace Microsoft::WRL;

#define  D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#include <core/vendor/D3D12MemAlloc.h>
#include <dxc/dxcapi.h>

#include "directx12_utilities.hpp"

#include "directx12_command_queue.hpp"
#include "directx12_backend.h"
#include "directx12_pipeline_context.h"
#include "directx12_resources.hpp"
