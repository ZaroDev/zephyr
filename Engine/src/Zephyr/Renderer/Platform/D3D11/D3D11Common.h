#pragma once
#include <Zephyr/Core/PlatformDetection.h>
#ifdef PLATFORM_WINDOWS
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl.h>

using namespace Microsoft::WRL;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

#endif