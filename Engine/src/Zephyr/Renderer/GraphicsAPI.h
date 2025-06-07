#pragma once
#include <Zephyr/Core/BasicTypes.h>
#include <nvrhi/nvrhi.h>

namespace Zephyr
{
	enum class GraphicsAPI : u8
	{
		D3D11,
		D3D12,
		VULKAN
	};

	nvrhi::GraphicsAPI ToNVRHI(GraphicsAPI api);
	StrView GetGraphicsName(GraphicsAPI api);
}
