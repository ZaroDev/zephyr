#include <pch.h>
#include "GraphicsAPI.h"

#include <cctype>

namespace Zephyr
{
	static bool EqualsIgnoreCase(StrView a, StrView b)
	{
		if (a.size() != b.size())
			return false;

		for (SizeT i = 0; i < a.size(); i++)
		{
			if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
				return false;
		}

		return true;
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

	nvrhi::GraphicsAPI ToNVRHI(GraphicsAPI api)
	{
		switch (api)
		{
		case GraphicsAPI::D3D11: return nvrhi::GraphicsAPI::D3D11;
		case GraphicsAPI::D3D12: return nvrhi::GraphicsAPI::D3D12;
		case GraphicsAPI::VULKAN: return nvrhi::GraphicsAPI::VULKAN;
		}

		CORE_ASSERT(false, "Unknown GraphicsAPI");
		return nvrhi::GraphicsAPI::VULKAN;
	}

	bool TryParseGraphicsAPI(StrView name, GraphicsAPI& outApi)
	{
		if (EqualsIgnoreCase(name, "d3d11") || EqualsIgnoreCase(name, "dx11"))
		{
			outApi = GraphicsAPI::D3D11;
			return true;
		}
		if (EqualsIgnoreCase(name, "d3d12") || EqualsIgnoreCase(name, "dx12"))
		{
			outApi = GraphicsAPI::D3D12;
			return true;
		}
		if (EqualsIgnoreCase(name, "vulkan") || EqualsIgnoreCase(name, "vk"))
		{
			outApi = GraphicsAPI::VULKAN;
			return true;
		}

		return false;
	}
}