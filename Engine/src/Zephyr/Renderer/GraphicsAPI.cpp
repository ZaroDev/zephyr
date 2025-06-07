#include <pch.h>
#include "GraphicsAPI.h"

namespace Zephyr
{
	nvrhi::GraphicsAPI ToNVRHI(GraphicsAPI api)
	{
		switch (api)
		{
		case GraphicsAPI::D3D11: return nvrhi::GraphicsAPI::D3D11;
		case GraphicsAPI::D3D12: return nvrhi::GraphicsAPI::D3D12;
		case GraphicsAPI::VULKAN: return nvrhi::GraphicsAPI::VULKAN;
		}
		return nvrhi::GraphicsAPI::VULKAN;
	}

	StrView GetGraphicsName(GraphicsAPI api)
	{
		switch (api)
		{
		case GraphicsAPI::D3D11: return "D3D11";
		case GraphicsAPI::D3D12: return "D3D12";
		case GraphicsAPI::VULKAN: return "Vulkan";
		}

		return "Unknown API";
	}
}