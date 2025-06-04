#pragma once
#include <Zephyr/Core/BasicTypes.h>
namespace Zephyr
{
	enum class GraphicsAPI : u8
	{
		D3D11,
		D3D12,
		VULKAN,
		MAX
	};

	StrView GetGraphicsName(GraphicsAPI api);
}