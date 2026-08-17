#include "pch.h"
#include "D3D12Device.h"

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

	D3D12Device::D3D12Device()
	{
		if (!CreateFactory())
		{
			CORE_CRITICAL("Failed to create a DXGI factory");
			return;
		}

		if (!CreateDeviceAndQueue())
		{
			CORE_CRITICAL("Failed to create a D3D12 device");
			return;
		}
	}

	D3D12Device::~D3D12Device()
	{
		m_NvrhiDevice = nullptr;
		m_GraphicsQueue = nullptr;
		m_Device = nullptr;
		m_DxgiAdapter = nullptr;
		m_DxgiFactory = nullptr;
	}

	Ref<Surface> D3D12Device::CreateSurface(const Ref<Window>& window)
	{
		// D3D12Surface lands in Task 13 - mirrors D3D11Device::CreateSurface()'s pre-Task-10 stub.
		return Ref<Surface>();
	}

	bool D3D12Device::CreateFactory()
	{
		const HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_DxgiFactory));
		if (FAILED(hr))
		{
			CORE_ERROR("ERROR in CreateDXGIFactory2, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		return true;
	}

	bool D3D12Device::CreateDeviceAndQueue()
	{
#ifdef DEBUG
		{
			nvrhi::RefCountPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
#endif

		// Adapter selection: use the first adapter, matching D3D11Device's default
		// (no adapter-index selection is exposed yet - see tasks/plan.md).
		if (FAILED(m_DxgiFactory->EnumAdapters(0, &m_DxgiAdapter)))
		{
			CORE_ERROR("Cannot find any DXGI adapters in the system.");
			return false;
		}

		DXGI_ADAPTER_DESC adapterDesc;
		m_DxgiAdapter->GetDesc(&adapterDesc);
		m_RendererString = GetAdapterName(adapterDesc);

		HRESULT hr = D3D12CreateDevice(m_DxgiAdapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_Device));
		if (FAILED(hr))
		{
			CORE_ERROR("D3D12CreateDevice failed, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_GraphicsQueue));
		if (FAILED(hr))
		{
			CORE_ERROR("Failed to create the D3D12 graphics command queue, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		nvrhi::d3d12::DeviceDesc deviceDesc;
		deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
		deviceDesc.pDevice = m_Device;
		deviceDesc.pGraphicsCommandQueue = m_GraphicsQueue;

		m_NvrhiDevice = nvrhi::d3d12::createDevice(deviceDesc);

#ifdef DEBUG
		m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
#endif

		CORE_INFO("Created D3D12 Device: {}", m_RendererString.c_str());

		return true;
	}
}
