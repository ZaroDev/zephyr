#include <pch.h>
#include "D3D11DeviceManager.h"

#pragma lib()

#include <Windows.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#include <nvrhi/d3d11.h>
#include <nvrhi/validation.h>

namespace Zephyr
{
    // Adjust window rect so that it is centred on the given adapter.  Clamps to fit if it's too big.
    static bool MoveWindowOntoAdapter(IDXGIAdapter* targetAdapter, RECT& rect)
    {
        assert(targetAdapter != NULL);

        HRESULT hres = S_OK;
        unsigned int outputNo = 0;
        while (SUCCEEDED(hres))
        {
            nvrhi::RefCountPtr<IDXGIOutput> pOutput;
            hres = targetAdapter->EnumOutputs(outputNo++, &pOutput);

            if (SUCCEEDED(hres) && pOutput)
            {
                DXGI_OUTPUT_DESC OutputDesc;
                pOutput->GetDesc(&OutputDesc);
                const RECT desktop = OutputDesc.DesktopCoordinates;
                const int centreX = (int)desktop.left + (int)(desktop.right - desktop.left) / 2;
                const int centreY = (int)desktop.top + (int)(desktop.bottom - desktop.top) / 2;
                const int winW = rect.right - rect.left;
                const int winH = rect.bottom - rect.top;
                const int left = centreX - winW / 2;
                const int right = left + winW;
                const int top = centreY - winH / 2;
                const int bottom = top + winH;
                rect.left = std::max(left, (int)desktop.left);
                rect.right = std::min(right, (int)desktop.right);
                rect.bottom = std::min(bottom, (int)desktop.bottom);
                rect.top = std::max(top, (int)desktop.top);

                // If there is more than one output, go with the first found.  Multi-monitor support could go here.
                return true;
            }
        }

        return false;
    }


    bool D3D11DeviceManager::BeginFrame()
    {
        DXGI_SWAP_CHAIN_DESC newSwapChainDesc;
        if (SUCCEEDED(m_SwapChain->GetDesc(&newSwapChainDesc)))
        {
            if (m_SwapChainDesc.Windowed != newSwapChainDesc.Windowed)
            {
                BackBufferResizing();

                m_SwapChainDesc = newSwapChainDesc;
                m_DeviceParams.BackBufferWidth = newSwapChainDesc.BufferDesc.Width;
                m_DeviceParams.BackBufferHeight = newSwapChainDesc.BufferDesc.Height;

                if (newSwapChainDesc.Windowed)
                    glfwSetWindowMonitor(m_Window, nullptr, 50, 50, newSwapChainDesc.BufferDesc.Width, newSwapChainDesc.BufferDesc.Height, 0);

                ResizeSwapChain();
                BackBufferResized();
            }
        }
        return true;
    }

    void D3D11DeviceManager::ReportLiveObjects()
    {
        nvrhi::RefCountPtr<IDXGIDebug> pDebug;
        DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug));

        if (pDebug)
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
    }

    bool D3D11DeviceManager::CreateInstanceInternal()
    {
        if (!m_DxgiFactory)
        {
            HRESULT hres = CreateDXGIFactory1(IID_PPV_ARGS(&m_DxgiFactory));
            if (hres != S_OK)
            {
               CORE_ERROR("ERROR in CreateDXGIFactory1.\n"
                    "For more info, get log from debug D3D runtime: (1) Install DX SDK, and enable Debug D3D from DX Control Panel Utility. (2) Install and start DbgView. (3) Try running the program again.\n");
                return false;
            }
        }

        return true;
    }

    bool D3D11DeviceManager::EnumerateAdapters(std::vector<AdapterInfo>& outAdapters)
    {
        if (!m_DxgiFactory)
            return false;

        outAdapters.clear();

        while (true)
        {
            nvrhi::RefCountPtr<IDXGIAdapter> adapter;
            HRESULT hr = m_DxgiFactory->EnumAdapters(uint32_t(outAdapters.size()), &adapter);
            if (FAILED(hr))
                return true;

            DXGI_ADAPTER_DESC desc;
            hr = adapter->GetDesc(&desc);
            if (FAILED(hr))
                return false;

            AdapterInfo adapterInfo;

            adapterInfo.Name = GetAdapterName(desc);
            adapterInfo.DXGIAdapter = adapter;
            adapterInfo.VendorID = desc.VendorId;
            adapterInfo.DeviceID = desc.DeviceId;
            adapterInfo.DedicatedVideoMemory = desc.DedicatedVideoMemory;

            AdapterInfo::LUID luid;
            static_assert(luid.size() == sizeof(desc.AdapterLuid));
            memcpy(luid.data(), &desc.AdapterLuid, luid.size());
            adapterInfo.Luid = luid;

            outAdapters.push_back(std::move(adapterInfo));
        }
    }

    bool D3D11DeviceManager::CreateDevice()
    {
        int adapterIndex = m_DeviceParams.AdapterIndex;

        if (adapterIndex < 0)
        {
            adapterIndex = 0;
        }

        if (FAILED(m_DxgiFactory->EnumAdapters(adapterIndex, &m_DxgiAdapter)))
        {
            if (adapterIndex == 0)
            {
               CORE_ERROR("Cannot find any DXGI adapters in the system.");
            }
            else
            {
                CORE_ERROR("The specified DXGI adapter {} does not exist.", adapterIndex);
            }
            return false;
        }

        DXGI_ADAPTER_DESC aDesc;

        m_DxgiAdapter->GetDesc(&aDesc);

        m_RendererString = GetAdapterName(aDesc);

        UINT createFlags = 0;
        if (m_DeviceParams.EnableDebugRuntime)
        {
            createFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
        const HRESULT hr = D3D11CreateDevice(
            m_DxgiAdapter, // pAdapter
            D3D_DRIVER_TYPE_UNKNOWN, // DriverType
            nullptr, // Software
            createFlags, // Flags
            &featureLevel, // pFeatureLevels
            1, // FeatureLevels
            D3D11_SDK_VERSION, // SDKVersion
            &m_Device, // ppDevice
            nullptr, // pFeatureLevel
            &m_ImmediateContext // ppImmediateContext
        );

        if (FAILED(hr))
        {
            return false;
        }

        nvrhi::d3d11::DeviceDesc deviceDesc;
        deviceDesc.messageCallback = &DefaultMessageCallback::GetInstance();
        deviceDesc.context = m_ImmediateContext;


        m_NvrhiDevice = nvrhi::d3d11::createDevice(deviceDesc);

        if (m_DeviceParams.EnableNvrhiValidationLayer)
        {
            m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
        }


        return true;
    }

    bool D3D11DeviceManager::CreateSwapChain()
    {
        UINT windowStyle = m_DeviceParams.StartFullscreen
            ? (WS_POPUP | WS_SYSMENU | WS_VISIBLE)
            : m_DeviceParams.StartMaximized
            ? (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE)
            : (WS_OVERLAPPEDWINDOW | WS_VISIBLE);

        RECT rect = { 0, 0, LONG(m_DeviceParams.BackBufferWidth), LONG(m_DeviceParams.BackBufferHeight) };
        AdjustWindowRect(&rect, windowStyle, FALSE);

        if (MoveWindowOntoAdapter(m_DxgiAdapter, rect))
        {
            glfwSetWindowPos(m_Window, rect.left, rect.top);
        }

        m_hWnd = glfwGetWin32Window(m_Window);

        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);
        UINT width = clientRect.right - clientRect.left;
        UINT height = clientRect.bottom - clientRect.top;
        DXGI_USAGE swapChainUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        ZeroMemory(&m_SwapChainDesc, sizeof(m_SwapChainDesc));
        m_SwapChainDesc.BufferCount = m_DeviceParams.SwapChainBufferCount;
        m_SwapChainDesc.BufferDesc.Width = width;
        m_SwapChainDesc.BufferDesc.Height = height;
        m_SwapChainDesc.BufferDesc.RefreshRate.Numerator = m_DeviceParams.RefreshRate;
        m_SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
        m_SwapChainDesc.BufferUsage = swapChainUsage;
        m_SwapChainDesc.OutputWindow = m_hWnd;
        m_SwapChainDesc.SampleDesc.Count = m_DeviceParams.SwapChainSampleCount;
        m_SwapChainDesc.SampleDesc.Quality = m_DeviceParams.SwapChainSampleQuality;
        m_SwapChainDesc.Windowed = !m_DeviceParams.StartFullscreen;
        m_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        m_SwapChainDesc.Flags = m_DeviceParams.AllowModeSwitch ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;

        // Special processing for sRGB swap chain formats.
        // DXGI will not create a swap chain with an sRGB format, but its contents will be interpreted as sRGB.
        // So we need to use a non-sRGB format here, but store the true sRGB format for later framebuffer creation.
        switch (m_DeviceParams.SwapChainFormat)  // NOLINT(clang-diagnostic-switch-enum)
        {
        case nvrhi::Format::SRGBA8_UNORM:
            m_SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case nvrhi::Format::SBGRA8_UNORM:
            m_SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            break;
        default:
            m_SwapChainDesc.BufferDesc.Format = nvrhi::d3d11::convertFormat(m_DeviceParams.SwapChainFormat);
            break;
        }

        HRESULT hr = m_DxgiFactory->CreateSwapChain(m_Device, &m_SwapChainDesc, &m_SwapChain);

        if (FAILED(hr))
        {
            CORE_ERROR("Failed to create a swap chain, HRESULT = 0x{}", hr);
            return false;
        }

        bool ret = CreateRenderTarget();

        if (!ret)
        {
            return false;
        }

        return true;
    }

    void D3D11DeviceManager::DestroyDeviceAndSwapChain()
    {
        m_RhiBackBuffer = nullptr;
        m_NvrhiDevice = nullptr;

        if (m_SwapChain)
        {
            m_SwapChain->SetFullscreenState(false, nullptr);
        }

        ReleaseRenderTarget();

        m_SwapChain = nullptr;
        m_ImmediateContext = nullptr;
        m_Device = nullptr;
    }

    bool D3D11DeviceManager::CreateRenderTarget()
    {
        ReleaseRenderTarget();

        const HRESULT hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&m_D3D11BackBuffer);  // NOLINT(clang-diagnostic-language-extension-token)
        if (FAILED(hr))
        {
            return false;
        }

        nvrhi::TextureDesc textureDesc;
        textureDesc.width = m_DeviceParams.BackBufferWidth;
        textureDesc.height = m_DeviceParams.BackBufferHeight;
        textureDesc.sampleCount = m_DeviceParams.SwapChainSampleCount;
        textureDesc.sampleQuality = m_DeviceParams.SwapChainSampleQuality;
        textureDesc.format = m_DeviceParams.SwapChainFormat;
        textureDesc.debugName = "SwapChainBuffer";
        textureDesc.isRenderTarget = true;
        textureDesc.isUAV = false;

        m_RhiBackBuffer = m_NvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D11_Resource, static_cast<ID3D11Resource*>(m_D3D11BackBuffer.Get()), textureDesc);

        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    void D3D11DeviceManager::ReleaseRenderTarget()
    {
        m_RhiBackBuffer = nullptr;
        m_D3D11BackBuffer = nullptr;
    }

    void D3D11DeviceManager::ResizeSwapChain()
    {
        ReleaseRenderTarget();

        if (!m_SwapChain)
            return;

        const HRESULT hr = m_SwapChain->ResizeBuffers(m_DeviceParams.SwapChainBufferCount,
            m_DeviceParams.BackBufferWidth,
            m_DeviceParams.BackBufferHeight,
            m_SwapChainDesc.BufferDesc.Format,
            m_SwapChainDesc.Flags);

        if (FAILED(hr))
        {
            CORE_ERROR("ResizeBuffers failed");
        }

        const bool ret = CreateRenderTarget();
        if (!ret)
        {
            CORE_ERROR("CreateRenderTarget failed");
        }
    }

    void D3D11DeviceManager::Shutdown()
    {
        DeviceManager::Shutdown();

        if (m_DeviceParams.EnableDebugRuntime)
        {
            ReportLiveObjects();
        }
    }

    bool D3D11DeviceManager::Present()
    {
        HRESULT result = m_SwapChain->Present(m_DeviceParams.VsyncEnabled ? 1 : 0, 0);
        return SUCCEEDED(result);
    }

    DeviceManager* DeviceManager::CreateD3D11()
    {
        return new D3D11DeviceManager();
    }


}