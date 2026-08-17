#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <queue>
#include <unordered_set>
#include <vulkan/vulkan.hpp>
#include "nvrhi/vulkan.h"

#include <Zephyr/RHI/Device.h>

namespace Zephyr
{
	class VulkanDevice final : public Device
	{
	public:
		VulkanDevice();
		~VulkanDevice();


		virtual GraphicsAPI GetGraphicsAPI() const { return GraphicsAPI::VULKAN; }
		virtual Ref<Surface> CreateSurface(const Ref<Window>& window);
		virtual nvrhi::DeviceHandle GetNvrhiDevice() const { return m_ValidationLayer ? m_ValidationLayer : nvrhi::DeviceHandle(m_NvrhiDevice); }

	private:
		void CreateVulkanInstance();

		vk::Instance m_VulkanInstance;
		vk::DebugReportCallbackEXT m_DebugReportCallback;
		vk::PhysicalDevice m_VulkanPhysicalDevice;

        nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
        nvrhi::DeviceHandle m_ValidationLayer;

#if VK_HEADER_VERSION >= 301
        typedef vk::detail::DynamicLoader VulkanDynamicLoader;
#else
        typedef vk::DynamicLoader VulkanDynamicLoader;
#endif
	};
}