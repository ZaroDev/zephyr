#pragma once

namespace Zephyr
{
	enum class GraphicsAPI
	{
		OPENGL,
		DX11,

		MAX
	};

	std::string_view GetGraphicsName(GraphicsAPI api);
}