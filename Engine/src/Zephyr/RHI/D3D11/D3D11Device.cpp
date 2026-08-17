#include "pch.h"
#include "D3D11Device.h"
#include "D3D11Surface.h"

#include <nvrhi/validation.h>

namespace Zephyr
{
	static std::string GetAdapterName(DXGI_ADAPTER_DESC const& desc)
	{
		size_t length = wcsnlen(desc.Description, _countof(desc.Description));

		std::string name;
		name.resize(length);
		WideCharToMultiByte(CP_ACP, 0, desc.Description, int(length), name.data(), int(name.size()), nullptr, nullptr);

		return name;
	}

	D3D11Device::D3D11Device()
	{
		if (!CreateFactory())
		{
			CORE_CRITICAL("Failed to create a DXGI factory");
			return;
		}

		if (!CreateDeviceAndContext())
		{
			CORE_CRITICAL("Failed to create a D3D11 device");
			return;
		}
	}

	D3D11Device::~D3D11Device()
	{
		m_NvrhiDevice = nullptr;
		m_ImmediateContext = nullptr;
		m_Device = nullptr;
		m_DxgiAdapter = nullptr;
		m_DxgiFactory = nullptr;
	}

	Ref<Surface> D3D11Device::CreateSurface(const Ref<Window>& window)
	{
		return CreateRef<D3D11Surface>(this, window);
	}

	bool D3D11Device::CreateFactory()
	{
		const HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_DxgiFactory));
		if (FAILED(hr))
		{
			CORE_ERROR("ERROR in CreateDXGIFactory1.\n"
				"For more info, get log from debug D3D runtime: (1) Install DX SDK, and enable Debug D3D from DX Control Panel Utility. (2) Install and start DbgView. (3) Try running the program again.\n");
			return false;
		}

		return true;
	}

	bool D3D11Device::CreateDeviceAndContext()
	{
		// Adapter selection: use the first adapter. No adapter-index selection is exposed yet -
		// see tasks/plan.md.
		if (FAILED(m_DxgiFactory->EnumAdapters(0, &m_DxgiAdapter)))
		{
			CORE_ERROR("Cannot find any DXGI adapters in the system.");
			return false;
		}

		DXGI_ADAPTER_DESC adapterDesc;
		m_DxgiAdapter->GetDesc(&adapterDesc);
		m_RendererString = GetAdapterName(adapterDesc);

		UINT createFlags = 0;
#ifdef DEBUG
		createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		const HRESULT hr = D3D11CreateDevice(
			m_DxgiAdapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createFlags,
			&featureLevel,
			1,
			D3D11_SDK_VERSION,
			&m_Device,
			nullptr,
			&m_ImmediateContext
		);

		if (FAILED(hr))
		{
			CORE_ERROR("D3D11CreateDevice failed, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		nvrhi::d3d11::DeviceDesc deviceDesc;
		deviceDesc.messageCallback = &DefaultMessageCallback::GetInstance();
		deviceDesc.context = m_ImmediateContext;

		m_NvrhiDevice = nvrhi::d3d11::createDevice(deviceDesc);

#ifdef DEBUG
		m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
#endif

		CORE_INFO("Created D3D11 Device: {}", m_RendererString.c_str());

		return true;
	}
}
