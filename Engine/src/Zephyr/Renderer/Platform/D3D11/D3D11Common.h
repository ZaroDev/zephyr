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

inline Zephyr::String Win32ErrorMessage(u32 error)
{
    if (error == 0) {
        return "Ok";
    }

    const DWORD formatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER //
        | FORMAT_MESSAGE_FROM_SYSTEM     //
        | FORMAT_MESSAGE_IGNORE_INSERTS;

    LPSTR buffer = nullptr;
    size size = FormatMessageA(formatFlags, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //
        (LPSTR)&buffer, 0, NULL);

    Zephyr::String message = fmt::format("{:#x} {}", error, Zephyr::StrView(buffer, size));
    LocalFree(buffer);
    return message;
}

#endif