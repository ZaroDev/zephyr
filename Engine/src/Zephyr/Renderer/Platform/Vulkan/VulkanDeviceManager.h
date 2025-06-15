/*
MIT License

Copyright (c) 2025 ZaroDev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <Zephyr/Renderer/DeviceManager.h>


#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <queue>
#include <unordered_set>
#include <vulkan/vulkan.hpp>
#include "nvrhi/vulkan.h"


namespace Zephyr
{
	class VulkanDeviceManager : public DeviceManager
	{
	public:
		NODISCARD nvrhi::IDevice* GetDevice() const override
		{
			if (m_ValidationLayer)
			{
				return m_ValidationLayer;
			}

			return m_NvrhiDevice;
		}

		NODISCARD nvrhi::GraphicsAPI GetGraphicsAPI() const override
		{
			return nvrhi::GraphicsAPI::VULKAN;
		}

		bool EnumerateAdapters(std::vector<AdapterInfo>& outAdapters) override;
		const DeviceCreationParameters& GetDeviceParams() const { return m_DeviceParams; }

	protected:
		bool CreateInstanceInternal() override;
		bool CreateDevice() override;
		bool CreateSwapChain() override;
		void DestroyDeviceAndSwapChain() override;

		void ResizeSwapChain() override
		{
			if (m_VulkanDevice)
			{
				destroySwapChain();
				createSwapChain();
			}
		}
        nvrhi::ITexture* GetCurrentBackBuffer() override
        {
            return m_SwapChainImages[m_SwapChainIndex].rhiHandle;
        }
        nvrhi::ITexture* GetBackBuffer(u32 index) override
        {
            if (index < m_SwapChainImages.size())
                return m_SwapChainImages[index].rhiHandle;
            return nullptr;
        }
        u32 GetCurrentBackBufferIndex() override
        {
            return m_SwapChainIndex;
        }
        u32 GetBackBufferCount() override
        {
            return u32(m_SwapChainImages.size());
        }

        bool BeginFrame() override;
        bool Present() override;

        const char* GetRendererString() const override
        {
            return m_RendererString.c_str();
        }

        bool IsVulkanInstanceExtensionEnabled(const char* extensionName) const override
        {
            return m_EnabledExtensions.Instance.find(extensionName) != m_EnabledExtensions.Instance.end();
        }

        bool IsVulkanDeviceExtensionEnabled(const char* extensionName) const override
        {
            return m_EnabledExtensions.Device.find(extensionName) != m_EnabledExtensions.Device.end();
        }

        bool IsVulkanLayerEnabled(const char* layerName) const override
        {
            return m_EnabledExtensions.Layers.find(layerName) != m_EnabledExtensions.Layers.end();
        }

        void GetEnabledVulkanInstanceExtensions(std::vector<std::string>& extensions) const override
        {
            for (const auto& ext : m_EnabledExtensions.Instance)
                extensions.push_back(ext);
        }

        void GetEnabledVulkanDeviceExtensions(std::vector<std::string>& extensions) const override
        {
            for (const auto& ext : m_EnabledExtensions.Device)
                extensions.push_back(ext);
        }

        void GetEnabledVulkanLayers(std::vector<std::string>& layers) const override
        {
            for (const auto& ext : m_EnabledExtensions.Layers)
                layers.push_back(ext);
        }

        bool createInstance();
        bool createWindowSurface();
        void installDebugCallback();
        bool pickPhysicalDevice();
        bool findQueueFamilies(vk::PhysicalDevice physicalDevice);
        bool createDevice();
        bool createSwapChain();
        void destroySwapChain();

        struct VulkanExtensionSet
        {
            std::unordered_set<std::string> Instance;
            std::unordered_set<std::string> Layers;
            std::unordered_set<std::string> Device;
        };

        // minimal set of required extensions
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

        // optional extensions
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

        std::unordered_set<std::string> m_RayTracingExtensions = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        };

        std::string m_RendererString;

        vk::Instance m_VulkanInstance;
        vk::DebugReportCallbackEXT m_DebugReportCallback;

        vk::PhysicalDevice m_VulkanPhysicalDevice;
        int m_GraphicsQueueFamily = -1;
        int m_ComputeQueueFamily = -1;
        int m_TransferQueueFamily = -1;
        int m_PresentQueueFamily = -1;

        vk::Device m_VulkanDevice;
        vk::Queue m_GraphicsQueue;
        vk::Queue m_ComputeQueue;
        vk::Queue m_TransferQueue;
        vk::Queue m_PresentQueue;

        vk::SurfaceKHR m_WindowSurface;

        vk::SurfaceFormatKHR m_SwapChainFormat;
        vk::SwapchainKHR m_SwapChain;
        bool m_SwapChainMutableFormatSupported = false;

        struct SwapChainImage
        {
            vk::Image image;
            nvrhi::TextureHandle rhiHandle;
        };

        std::vector<SwapChainImage> m_SwapChainImages;
        u32 m_SwapChainIndex = u32(-1);

        nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
        nvrhi::DeviceHandle m_ValidationLayer;

        std::vector<vk::Semaphore> m_AcquireSemaphores;
        std::vector<vk::Semaphore> m_PresentSemaphores;
        u32 m_AcquireSemaphoreIndex = 0;
        u32 m_PresentSemaphoreIndex = 0;

        std::queue<nvrhi::EventQueryHandle> m_FramesInFlight;
        std::vector<nvrhi::EventQueryHandle> m_QueryPool;

        bool m_BufferDeviceAddressSupported = false;

#if VK_HEADER_VERSION >= 301
        typedef vk::detail::DynamicLoader VulkanDynamicLoader;
#else
        typedef vk::DynamicLoader VulkanDynamicLoader;
#endif

        std::unique_ptr<VulkanDynamicLoader> m_DynamicLoader;
	};
}
