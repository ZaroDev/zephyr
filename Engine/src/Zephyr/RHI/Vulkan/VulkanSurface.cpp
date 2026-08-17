#include "pch.h"
#include "VulkanSurface.h"
#include "VulkanDevice.h"

#include <unordered_set>

#include <GLFW/glfw3.h>

namespace Zephyr
{
	static constexpr nvrhi::Format c_SwapChainFormat = nvrhi::Format::SBGRA8_UNORM;

	VulkanSurface::VulkanSurface(VulkanDevice* device, const Ref<Window>& window)
		: m_Device(device), m_Window(window)
	{
		int w, h;
		glfwGetFramebufferSize(window->GetGLFWHandle(), &w, &h);
		m_Width = u32(w);
		m_Height = u32(h);

		if (!CreateWindowSurface())
		{
			CORE_CRITICAL("Failed to create a Vulkan window surface");
			return;
		}

		if (!CreateSwapChain())
		{
			CORE_CRITICAL("Failed to create a Vulkan swap chain");
			return;
		}

		CreateSemaphores();
	}

	VulkanSurface::~VulkanSurface()
	{
		DestroySwapChain();

		vk::Device device = m_Device->GetVulkanDevice();
		for (auto& semaphore : m_PresentSemaphores)
		{
			if (semaphore)
				device.destroySemaphore(semaphore);
		}
		for (auto& semaphore : m_AcquireSemaphores)
		{
			if (semaphore)
				device.destroySemaphore(semaphore);
		}

		if (m_WindowSurface)
		{
			m_Device->GetVulkanInstance().destroySurfaceKHR(m_WindowSurface);
			m_WindowSurface = nullptr;
		}
	}

	void VulkanSurface::Resize(u32 width, u32 height)
	{
		if (width == m_Width && height == m_Height)
			return;

		m_Width = width;
		m_Height = height;
		CreateSwapChain();
	}

	bool VulkanSurface::CreateWindowSurface()
	{
		VkSurfaceKHR surface;
		const VkResult res = glfwCreateWindowSurface(VkInstance(m_Device->GetVulkanInstance()), m_Window->GetGLFWHandle(), nullptr, &surface);
		if (res != VK_SUCCESS)
		{
			CORE_ERROR("Failed to create a GLFW window surface, error code = {}", nvrhi::vulkan::resultToString(res));
			return false;
		}

		m_WindowSurface = vk::SurfaceKHR(surface);
		return true;
	}

	void VulkanSurface::DestroySwapChain()
	{
		vk::Device device = m_Device->GetVulkanDevice();
		if (device)
		{
			device.waitIdle();
		}

		m_SwapChainFramebuffers.clear();

		if (m_SwapChain)
		{
			device.destroySwapchainKHR(m_SwapChain);
			m_SwapChain = nullptr;
		}

		m_SwapChainImages.clear();
	}

	bool VulkanSurface::CreateSwapChain()
	{
		DestroySwapChain();

		m_SwapChainFormat = {
			vk::Format(nvrhi::vulkan::convertFormat(c_SwapChainFormat)),
			vk::ColorSpaceKHR::eSrgbNonlinear
		};

		const vk::Extent2D extent(m_Width, m_Height);

		std::unordered_set<u32> uniqueQueues = {
			m_Device->GetGraphicsQueueFamily(),
			m_Device->GetPresentQueueFamily()
		};
		std::vector<u32> queues(uniqueQueues.begin(), uniqueQueues.end());
		const bool enableSwapChainSharing = queues.size() > 1;

		const bool mutableFormatSupported = m_Device->IsSwapChainMutableFormatSupported();

		auto desc = vk::SwapchainCreateInfoKHR()
			.setSurface(m_WindowSurface)
			.setMinImageCount(c_FramesInFlight)
			.setImageFormat(m_SwapChainFormat.format)
			.setImageColorSpace(m_SwapChainFormat.colorSpace)
			.setImageExtent(extent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			.setImageSharingMode(enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
			.setFlags(mutableFormatSupported ? vk::SwapchainCreateFlagBitsKHR::eMutableFormat : vk::SwapchainCreateFlagBitsKHR(0))
			.setQueueFamilyIndexCount(enableSwapChainSharing ? u32(queues.size()) : 0)
			.setPQueueFamilyIndices(enableSwapChainSharing ? queues.data() : nullptr)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(vk::PresentModeKHR::eImmediate)
			.setClipped(true)
			.setOldSwapchain(nullptr);

		// c_SwapChainFormat (SBGRA8_UNORM) maps to VK_FORMAT_B8G8R8A8_SRGB; the mutable-format
		// image view list needs the UNORM counterpart alongside it.
		std::vector<vk::Format> imageFormats = { m_SwapChainFormat.format, vk::Format::eB8G8R8A8Unorm };
		auto imageFormatListCreateInfo = vk::ImageFormatListCreateInfo().setViewFormats(imageFormats);
		if (mutableFormatSupported)
			desc.pNext = &imageFormatListCreateInfo;

		vk::Device device = m_Device->GetVulkanDevice();
		const vk::Result res = device.createSwapchainKHR(&desc, nullptr, &m_SwapChain);
		if (res != vk::Result::eSuccess)
		{
			CORE_ERROR("Failed to create a Vulkan swap chain, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
			return false;
		}

		nvrhi::vulkan::IDevice* rawNvrhiDevice = m_Device->GetRawNvrhiDevice();

		auto images = device.getSwapchainImagesKHR(m_SwapChain);
		for (auto image : images)
		{
			SwapChainImage sci;
			sci.Image = image;

			nvrhi::TextureDesc textureDesc;
			textureDesc.width = m_Width;
			textureDesc.height = m_Height;
			textureDesc.format = c_SwapChainFormat;
			textureDesc.debugName = "Swap chain image";
			textureDesc.initialState = nvrhi::ResourceStates::Present;
			textureDesc.keepInitialState = true;
			textureDesc.isRenderTarget = true;

			sci.RhiHandle = rawNvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.Image), textureDesc);
			m_SwapChainImages.push_back(sci);
		}

		nvrhi::IDevice* nvrhiDevice = m_Device->GetNvrhiDevice();
		m_SwapChainFramebuffers.resize(m_SwapChainImages.size());
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			m_SwapChainFramebuffers[i] = nvrhiDevice->createFramebuffer(
				nvrhi::FramebufferDesc().addColorAttachment(m_SwapChainImages[i].RhiHandle));
		}

		m_SwapChainIndex = 0;

		return true;
	}

	void VulkanSurface::CreateSemaphores()
	{
		vk::Device device = m_Device->GetVulkanDevice();
		m_AcquireSemaphores.reserve(c_FramesInFlight + 1);
		m_PresentSemaphores.reserve(c_FramesInFlight + 1);
		for (u32 i = 0; i < c_FramesInFlight + 1; ++i)
		{
			m_AcquireSemaphores.push_back(device.createSemaphore(vk::SemaphoreCreateInfo()));
			m_PresentSemaphores.push_back(device.createSemaphore(vk::SemaphoreCreateInfo()));
		}
	}

	bool VulkanSurface::AcquireNextImage()
	{
		vk::Device device = m_Device->GetVulkanDevice();
		const vk::Semaphore& semaphore = m_AcquireSemaphores[m_AcquireSemaphoreIndex];

		vk::Result res{};
		constexpr int maxAttempts = 3;
		for (int attempt = 0; attempt < maxAttempts; ++attempt)
		{
			res = device.acquireNextImageKHR(
				m_SwapChain,
				std::numeric_limits<uint64_t>::max(),
				semaphore,
				vk::Fence(),
				&m_SwapChainIndex);

			if ((res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) && attempt < maxAttempts - 1)
			{
				int w, h;
				glfwGetFramebufferSize(m_Window->GetGLFWHandle(), &w, &h);
				m_Width = u32(w);
				m_Height = u32(h);
				CreateSwapChain();
			}
			else
			{
				break;
			}
		}

		m_AcquireSemaphoreIndex = (m_AcquireSemaphoreIndex + 1) % u32(m_AcquireSemaphores.size());

		if (res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR)
		{
			m_Device->GetRawNvrhiDevice()->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);
			return true;
		}

		return false;
	}

	nvrhi::IFramebuffer* VulkanSurface::GetCurrentFramebuffer()
	{
		if (m_SwapChainIndex < m_SwapChainFramebuffers.size())
			return m_SwapChainFramebuffers[m_SwapChainIndex];

		return nullptr;
	}

	bool VulkanSurface::Present()
	{
		const vk::Semaphore& semaphore = m_PresentSemaphores[m_PresentSemaphoreIndex];
		nvrhi::vulkan::IDevice* rawNvrhiDevice = m_Device->GetRawNvrhiDevice();

		rawNvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);

		// NVRHI buffers the semaphore signal until something is submitted to the queue.
		rawNvrhiDevice->executeCommandLists(nullptr, 0);

		vk::PresentInfoKHR info = vk::PresentInfoKHR()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&semaphore)
			.setSwapchainCount(1)
			.setPSwapchains(&m_SwapChain)
			.setPImageIndices(&m_SwapChainIndex);

		const vk::Result res = m_Device->GetPresentQueue().presentKHR(&info);
		if (!(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR))
		{
			return false;
		}

		m_PresentSemaphoreIndex = (m_PresentSemaphoreIndex + 1) % u32(m_PresentSemaphores.size());

		return true;
	}
}
