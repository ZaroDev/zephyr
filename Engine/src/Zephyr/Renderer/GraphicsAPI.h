#pragma once

namespace Zephyr
{
	enum class GraphicsAPI
	{
		OPENGL,
		DX11,

		MAX
	};

	StrView GetGraphicsName(GraphicsAPI api);
}