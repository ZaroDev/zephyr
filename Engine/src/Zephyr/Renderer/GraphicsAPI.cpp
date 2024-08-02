#include <pch.h>
#include "GraphicsAPI.h"

namespace Zephyr
{
	std::string_view GetGraphicsName(GraphicsAPI api)
	{
		switch (api)
		{
		case Zephyr::GraphicsAPI::OPENGL: return "OpenGL";
		case Zephyr::GraphicsAPI::DX11: return "Direct3D11";
		}

		return "Unknown API";
	}
}