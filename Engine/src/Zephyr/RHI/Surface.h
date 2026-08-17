#pragma once

#include <Zephyr/Core/BasicTypes.h>

namespace Zephyr
{
	constexpr u32 c_FramesInFlight = 3;

	class Surface
	{
	public:
		Surface() = default;
		virtual ~Surface() = default;

		virtual u32 GetCurrentFrameIndex() const = 0;
		virtual u32 GetWidth() const = 0;
		virtual u32 GetHeight() const = 0;
		

		virtual void Resize(u32 width, u32 height) = 0;
	};
}