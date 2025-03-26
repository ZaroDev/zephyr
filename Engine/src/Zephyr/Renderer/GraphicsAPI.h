#pragma once
#include <Zephyr/Core/BasicTypes.h>
namespace Zephyr
{
	enum class GraphicsAPI : u8
	{
		VULKAN,
		MAX
	};

	StrView GetGraphicsName(GraphicsAPI api);
}