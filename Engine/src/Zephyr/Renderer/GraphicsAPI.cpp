#include <pch.h>
#include "GraphicsAPI.h"

namespace Zephyr
{
	StrView GetGraphicsName(GraphicsAPI api)
	{
		switch (api)
		{
		case Zephyr::GraphicsAPI::OPENGL: return "OpenGL";
		case Zephyr::GraphicsAPI::DX11: return "Direct3D11";
		}

		return "Unknown API";
	}
}