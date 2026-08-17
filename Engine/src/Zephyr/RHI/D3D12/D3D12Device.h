#pragma once

#include <Windows.h>
#include <dxgi1_4.h>

#include <nvrhi/d3d12.h>

#include <Zephyr/RHI/Device.h>

namespace Zephyr
{
	class D3D12Device final : public Device
	{
	public:
		D3D12Device();
		~D3D12Device();

		virtual GraphicsAPI GetGraphicsAPI() const override { return GraphicsAPI::D3D12; }
		virtual Ref<Surface> CreateSurface(const Ref<Window>& window) override;
		virtual nvrhi::DeviceHandle GetNvrhiDevice() const override { return m_NvrhiDevice; }

		// Accessors needed by RHI::D3D12Surface (owns the DXGI swap chain and per-frame fence
		// sync, which this Device knows nothing about - see tasks/plan.md Task 13, matching the
		// Device/Surface split D3D11Device/D3D11Surface already use).
		IDXGIFactory4* GetDxgiFactory() const { return m_DxgiFactory; }
		ID3D12Device* GetD3D12Device() const { return m_Device; }
		ID3D12CommandQueue* GetGraphicsQueue() const { return m_GraphicsQueue; }

	private:
		bool CreateFactory();
		bool CreateDeviceAndQueue();

		nvrhi::RefCountPtr<IDXGIFactory4> m_DxgiFactory;
		nvrhi::RefCountPtr<IDXGIAdapter> m_DxgiAdapter;
		nvrhi::RefCountPtr<ID3D12Device> m_Device;
		nvrhi::RefCountPtr<ID3D12CommandQueue> m_GraphicsQueue;

		nvrhi::DeviceHandle m_NvrhiDevice;

		std::string m_RendererString;
	};
}
