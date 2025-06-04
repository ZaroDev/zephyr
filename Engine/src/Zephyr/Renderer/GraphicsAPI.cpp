#include <pch.h>
#include "GraphicsAPI.h"

namespace Zephyr
{
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