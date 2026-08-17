#pragma once

#include <Zephyr/Core/BasicTypes.h>

#include <nvrhi/nvrhi.h>

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

		// Acquires the next swapchain image, recreating the swapchain first if needed. Must be
		// called once per frame before GetCurrentFramebuffer(). Returns false on unrecoverable failure.
		virtual bool AcquireNextImage() = 0;

		// The framebuffer wrapping the image last returned by AcquireNextImage().
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() = 0;

		// Presents the current image. Returns false on unrecoverable failure.
		virtual bool Present() = 0;
	};
}
