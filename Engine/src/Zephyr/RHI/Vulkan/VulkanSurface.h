#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <vulkan/vulkan.hpp>
#include "nvrhi/vulkan.h"

#include <Zephyr/RHI/Surface.h>
#include <Zephyr/Window/Window.h>

namespace Zephyr
{
	class VulkanDevice;

	class VulkanSurface final : public Surface
	{
	public:
		VulkanSurface(VulkanDevice* device, const Ref<Window>& window);
		~VulkanSurface();

		virtual u32 GetCurrentFrameIndex() const override { return m_SwapChainIndex; }
		virtual u32 GetWidth() const override { return m_Width; }
		virtual u32 GetHeight() const override { return m_Height; }
		virtual void Resize(u32 width, u32 height) override;

		virtual bool AcquireNextImage() override;
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() override;
		virtual bool Present() override;

	private:
		bool CreateWindowSurface();
		bool CreateSwapChain();
		void DestroySwapChain();
		void CreateSemaphores();

		struct SwapChainImage
		{
			vk::Image Image;
			nvrhi::TextureHandle RhiHandle;
		};

		VulkanDevice* m_Device = nullptr;
		Ref<Window> m_Window;

		u32 m_Width = 0;
		u32 m_Height = 0;

		vk::SurfaceKHR m_WindowSurface;
		vk::SurfaceFormatKHR m_SwapChainFormat;
		vk::SwapchainKHR m_SwapChain;

		std::vector<SwapChainImage> m_SwapChainImages;
		std::vector<nvrhi::FramebufferHandle> m_SwapChainFramebuffers;
		u32 m_SwapChainIndex = 0;

		std::vector<vk::Semaphore> m_AcquireSemaphores;
		std::vector<vk::Semaphore> m_PresentSemaphores;
		u32 m_AcquireSemaphoreIndex = 0;
		u32 m_PresentSemaphoreIndex = 0;
	};
}
