#include <pch.h>
#include "GraphicsAPI.h"

namespace Zephyr
{
	StrView GetGraphicsName(GraphicsAPI api)
	{
		switch (api)
		{
		case GraphicsAPI::VULKAN: return "Vulkan";
		}

		return "Unknown API";
	}
}