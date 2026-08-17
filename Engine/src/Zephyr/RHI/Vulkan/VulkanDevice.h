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

		// Accessors needed by RHI::VulkanSurface (owns the window surface/swapchain, which this
		// Device knows nothing about - see tasks/plan.md Task 5).
		vk::Instance GetVulkanInstance() const { return m_VulkanInstance; }
		vk::PhysicalDevice GetVulkanPhysicalDevice() const { return m_VulkanPhysicalDevice; }
		vk::Device GetVulkanDevice() const { return m_VulkanDevice; }
		u32 GetGraphicsQueueFamily() const { return u32(m_GraphicsQueueFamily); }
		u32 GetPresentQueueFamily() const { return u32(m_PresentQueueFamily); }
		vk::Queue GetGraphicsQueue() const { return m_GraphicsQueue; }
		vk::Queue GetPresentQueue() const { return m_PresentQueue; }
		bool IsSwapChainMutableFormatSupported() const { return m_SwapChainMutableFormatSupported; }

	private:
		bool CreateVulkanInstance();
		void InstallDebugCallback();
		bool PickPhysicalDevice();
		bool FindQueueFamilies(vk::PhysicalDevice physicalDevice);
		bool CreateLogicalDevice();

		struct VulkanExtensionSet
		{
			std::unordered_set<std::string> Instance;
			std::unordered_set<std::string> Layers;
			std::unordered_set<std::string> Device;
		};

		// Same minimal required-extension set VulkanDeviceManager used - kept verbatim rather
		// than "cleaned up" during the port, per tasks/plan.md's extension-drift risk note.
		VulkanExtensionSet m_EnabledExtensions = {
			// Instance
			{
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
			},
			// Layers
			{ },
			// Device
			{
				VK_KHR_MAINTENANCE1_EXTENSION_NAME
			},
		};

		VulkanExtensionSet m_OptionalExtensions = {
			// Instance
			{
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
				VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			},
			// Layers
			{ },
			// Device
			{
				VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
				VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
				VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
				VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
				VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
				VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
				VK_NV_MESH_SHADER_EXTENSION_NAME,
			},
		};

		std::string m_RendererString;

		vk::Instance m_VulkanInstance;
		vk::DebugReportCallbackEXT m_DebugReportCallback;
		vk::PhysicalDevice m_VulkanPhysicalDevice;

		int m_GraphicsQueueFamily = -1;
		int m_PresentQueueFamily = -1;

		vk::Device m_VulkanDevice;
		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

		bool m_BufferDeviceAddressSupported = false;
		bool m_SwapChainMutableFormatSupported = false;

		nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
		nvrhi::DeviceHandle m_ValidationLayer;

#if VK_HEADER_VERSION >= 301
		typedef vk::detail::DynamicLoader VulkanDynamicLoader;
#else
		typedef vk::DynamicLoader VulkanDynamicLoader;
#endif
		std::unique_ptr<VulkanDynamicLoader> m_DynamicLoader;

		friend class VulkanSurface;
	};
}
