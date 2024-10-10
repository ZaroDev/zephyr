#pragma once

namespace Zephyr
{
	struct RenderHardwareInterface;

	namespace OpenGL
	{
		void GetPlatformInterface(RenderHardwareInterface& rhi);
	}
}
