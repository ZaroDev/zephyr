#pragma once

#include <Windows.h>
#include <dxgi1_3.h>

#include <nvrhi/d3d11.h>

#include <Zephyr/RHI/Device.h>

namespace Zephyr
{
	class D3D11Device final : public Device
	{
	public:
		D3D11Device();
		~D3D11Device();

		virtual GraphicsAPI GetGraphicsAPI() const override { return GraphicsAPI::D3D11; }
		virtual Ref<Surface> CreateSurface(const Ref<Window>& window) override;
		virtual nvrhi::DeviceHandle GetNvrhiDevice() const override { return m_NvrhiDevice; }

		// Accessors needed by RHI::D3D11Surface (owns the DXGI swap chain, which this Device
		// knows nothing about - see tasks/plan.md Task 10, matching the Vulkan Device/Surface split).
		IDXGIFactory1* GetDxgiFactory() const { return m_DxgiFactory; }
		IDXGIAdapter* GetDxgiAdapter() const { return m_DxgiAdapter; }
		ID3D11Device* GetD3D11Device() const { return m_Device; }

	private:
		bool CreateFactory();
		bool CreateDeviceAndContext();

		nvrhi::RefCountPtr<IDXGIFactory1> m_DxgiFactory;
		nvrhi::RefCountPtr<IDXGIAdapter> m_DxgiAdapter;
		nvrhi::RefCountPtr<ID3D11Device> m_Device;
		nvrhi::RefCountPtr<ID3D11DeviceContext> m_ImmediateContext;

		nvrhi::DeviceHandle m_NvrhiDevice;

		std::string m_RendererString;
	};
}
