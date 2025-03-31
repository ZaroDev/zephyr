#include <pch.h>
#include "VulkanRHI.h"

#include "Core/Application.h"
#include "VulkanUtils.h"

#define VMA_IMPLEMENTATION
#include "VulkanShader.h"
#include "vma/vk_mem_alloc.h"

#include <ImGui/imgui_impl_glfw.h>
#include <ImGui/imgui_impl_vulkan.h>



#ifndef DIST
constexpr bool m_UseValidationLayers = true;
#else
constexpr bool m_UseValidationLayers = false;
#endif
namespace Zephyr
{
	bool VulkanRHI::Init()
	{
		bool ret = true;
		ret &= InitVulkan();
		ret &= InitSwapChain();
		ret &= InitCommands();
		ret &= InitSyncStructures();
		ret &= InitDescriptors();
		ret &= InitPipelines();

		return ret;
	}

	void VulkanRHI::Shutdown()
	{
		vkDeviceWaitIdle(m_Device);
		for (u32 i = 0; i < c_FrameOverlap; i++)
		{
			vkDestroyCommandPool(m_Device, m_Frames[i].CommandPool, nullptr);

			vkDestroyFence(m_Device, m_Frames[i].RenderFence, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].RenderSemaphore, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].SwapChainSemaphore, nullptr);

			m_Frames[i].DeletionQueue.Flush();
		}
		m_MainDeletionQueue.Flush();

		DestroySwapchain();

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

		vkDestroyDevice(m_Device, nullptr);

		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
		vkDestroyInstance(m_Instance, nullptr);

	}

	void VulkanRHI::OnResize(u32 width, u32 height)
	{
	}

	void VulkanRHI::BeginFrame()
	{
		// Wait until the GPU has finished rendering the last frame with a timeout of 1 second.
		VK_ASSERT(vkWaitForFences(m_Device, 1, &GetCurrentFrame().RenderFence, true, 1000000000));

		// Flush to delete everything from previous frame
		GetCurrentFrame().DeletionQueue.Flush();

		VK_ASSERT(vkResetFences(m_Device, 1, &GetCurrentFrame().RenderFence));

		VK_ASSERT(vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, GetCurrentFrame().SwapChainSemaphore, nullptr, &m_CurrentSwapchainImage));

		VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;
		VK_ASSERT(vkResetCommandBuffer(cmd, 0));

		m_DrawExtent.width = m_DrawImage.ImageExtent.width;
		m_DrawExtent.height = m_DrawImage.ImageExtent.height;

		VkCommandBufferBeginInfo cmdBeginInfo = Utils::CommandBufferBeginInfo();
		VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		Utils::TransitionImage(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		ClearBackBuffer(cmd);

		Utils::TransitionImage(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		Utils::TransitionImage(cmd, m_SwapchainImages[m_CurrentSwapchainImage], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		Utils::CopyImageToImage(cmd, m_DrawImage.Image, m_SwapchainImages[m_CurrentSwapchainImage], m_DrawExtent, m_SwapchainExtent);

		Utils::TransitionImage(cmd, m_SwapchainImages[m_CurrentSwapchainImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	void VulkanRHI::EndFrame()
	{
		VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;

		Utils::TransitionImage(cmd, m_SwapchainImages[m_CurrentSwapchainImage], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		VK_ASSERT(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdInfo = Utils::CommandBufferSubmitInfo(cmd);

		VkSemaphoreSubmitInfo waitInfo = Utils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().SwapChainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = Utils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().RenderSemaphore);

		VkSubmitInfo2 submit = Utils::SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

		VK_ASSERT(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, GetCurrentFrame().RenderFence));
		// Prepare present
		// This will put the image we just rendered to into the visible window.
		// We want to wait on the render semaphore for that,
		// as it's necessary that drawing commands have finished before the image is displayed to the user.
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &GetCurrentFrame().RenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &m_CurrentSwapchainImage;


		VK_ASSERT(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

		m_FrameNumber++;
	}

	bool VulkanRHI::ImGuiInit()
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
		};


		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 0;
		for (VkDescriptorPoolSize& pool_size : poolSizes)
			poolInfo.maxSets += pool_size.descriptorCount;
		poolInfo.poolSizeCount = (u32)std::size(poolSizes);
		poolInfo.pPoolSizes = poolSizes;

		VK_ASSERT(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_ImGuiPool));

		ImGui_ImplGlfw_InitForVulkan(Application::Get().GetWindow().GetGLFWWindow(), true);
		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = m_Instance;
		initInfo.PhysicalDevice = m_PhysicalDevice;
		initInfo.Device = m_Device;
		initInfo.Queue = m_GraphicsQueue;
		initInfo.QueueFamily = m_GraphicsQueueFamily;
		initInfo.DescriptorPool = m_ImGuiPool;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.UseDynamicRendering = true;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		initInfo.PipelineRenderingCreateInfo = {};
		initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainImageFormat;

		ImGui_ImplVulkan_Init(&initInfo);

		return true;
	}

	void VulkanRHI::ImGuiNewFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void VulkanRHI::ImGuiEndFrame()
	{
		VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;
		VkRenderingAttachmentInfo colorAttachment = Utils::AttachmentInfo(m_SwapchainImageViews[m_CurrentSwapchainImage], nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = Utils::RenderingInfo(m_SwapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	void VulkanRHI::ImGuiShutdown()
	{
		vkDeviceWaitIdle(m_Device);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		vkDestroyDescriptorPool(m_Device, m_ImGuiPool, nullptr);
	}

	void VulkanRHI::RebuildFontTextures() const
	{
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	bool VulkanRHI::InitVulkan()
	{

		// Create the vulkan instance with the default debug messenger
		vkb::InstanceBuilder builder;
		auto instanceReturn = builder.set_app_name(Application::Get().Specification().Name.c_str())
			.request_validation_layers(m_UseValidationLayers)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();
		vkb::Instance vkbInstance = instanceReturn.value();

		m_Instance = vkbInstance.instance;
		m_DebugMessenger = vkbInstance.debug_messenger;
		VK_INFO("Created instance!");

		// Create the surface


#ifdef PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = static_cast<HWND>(Application::Get().GetWindow().GetOSWindowPointer());
		createInfo.hinstance = GetModuleHandle(nullptr);
		if (vkCreateWin32SurfaceKHR(m_Instance, &createInfo, nullptr, &m_Surface) != VK_SUCCESS)
		{
			return false;
		}
#else
#error "Platform surface not implemented"
#endif
		VK_INFO("Created surface!");


		// Set up the feature set

		VkPhysicalDeviceVulkan13Features features{};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features.dynamicRendering = true;
		features.synchronization2 = true;

		VkPhysicalDeviceVulkan12Features features12{};
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		vkb::PhysicalDeviceSelector deviceSelector{ vkbInstance };
		auto selectorReturn = deviceSelector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(m_Surface)
			.select();

		vkb::DeviceBuilder deviceBuilder{ selectorReturn.value() };
		auto deviceReturn = deviceBuilder.build();

		vkb::Device device = deviceReturn.value();
		m_Device = device.device;
		m_PhysicalDevice = device.physical_device;

		VkPhysicalDeviceProperties2 deviceProperties{};
		deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProperties);

		VkPhysicalDeviceMemoryProperties2 memoryProperties{};
		memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		vkGetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &memoryProperties);


		m_RenderDeviceInfo.Name = String(deviceProperties.properties.deviceName);
		m_RenderDeviceInfo.PhysicalKind = static_cast<RenderDevice::Kind>(deviceProperties.properties.deviceType);
		m_RenderDeviceInfo.VendorId = deviceProperties.properties.deviceID;

		VK_INFO("Selected physical device");
		VK_INFO("Device name {0}", m_RenderDeviceInfo.Name.c_str());

		m_GraphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
		m_GraphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

		// Create the memory allocator
		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.device = m_Device;
		allocatorInfo.instance = m_Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		m_MainDeletionQueue.PushFunction([&]()
			{
				vmaDestroyAllocator(m_Allocator);
			});
		

		return VK_CHECK(instanceReturn.vk_result()) && VK_CHECK(deviceReturn.vk_result()) && VK_CHECK(deviceReturn.vk_result());
	}

	bool VulkanRHI::InitSwapChain()
	{
		const Window::WindowData& data = Application::Get().GetWindow().GetWindowData();
		VkExtent3D drawImageExtent =
		{
			data.Width,
			data.Height,
			1
		};

		m_DrawImage.ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		m_DrawImage.ImageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rImgInfo = Utils::ImageCreateInfo(m_DrawImage.ImageFormat, drawImageUsages, drawImageExtent);

		VmaAllocationCreateInfo rImgAllocInfo{};
		rImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vmaCreateImage(m_Allocator, &rImgInfo, &rImgAllocInfo, &m_DrawImage.Image, &m_DrawImage.Allocation, nullptr);

		VkImageViewCreateInfo rViewInfo = Utils::ImageViewCreateInfo(m_DrawImage.ImageFormat, m_DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		VK_ASSERT(vkCreateImageView(m_Device, &rViewInfo, nullptr, &m_DrawImage.ImageView));

		m_MainDeletionQueue.PushFunction([=]()
			{
				vkDestroyImageView(m_Device, m_DrawImage.ImageView, nullptr);
				vmaDestroyImage(m_Allocator, m_DrawImage.Image, m_DrawImage.Allocation);
			});

		return CreateSwapchain(data.Width, data.Height);
	}

	bool VulkanRHI::InitCommands()
	{
		VkCommandPoolCreateInfo commandPoolInfo = Utils::CommandPoolCreateInfo(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		for (u32 i = 0; i < c_FrameOverlap; i++)
		{
			VK_ASSERT(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool));

			VkCommandBufferAllocateInfo cmdAllocInfo = Utils::CommandBufferAllocateInfo(m_Frames[i].CommandPool);

			VK_ASSERT(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].MainCommandBuffer));
		}

		return true;
	}

	bool VulkanRHI::InitSyncStructures()
	{
		// Create synchronization structures
		// One fence to control when the gpu has finished rendering the frame,
		// and 2 semaphores to synchronize rendering with swapchain
		// we want the fence tto start signaled so we can wait on it on the first frame.
		VkFenceCreateInfo fenceCreateInfo = Utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = Utils::SemaphoreCreateInfo();

		for (u32 i = 0; i < c_FrameOverlap; i++)
		{
			VK_ASSERT(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence));
			VK_ASSERT(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapChainSemaphore));
			VK_ASSERT(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].RenderSemaphore));
		}
		return true;
	}

	bool VulkanRHI::InitDescriptors()
	{
		// Create a descriptor pool that will hold 10 sets with 1 image each
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
		};

		m_GlobalDescriptorAllocator.InitPool(m_Device, 10, sizes);

		// Make the descriptor set layout for our compute draw
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			m_DrawImageDescriptorLayout = builder.Build(m_Device, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		// Allocate a descriptor set for our draw image
		m_DrawImageDescriptors = m_GlobalDescriptorAllocator.Allocate(m_Device, m_DrawImageDescriptorLayout);

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = m_DrawImage.ImageView;

		VkWriteDescriptorSet drawImageWrite{};
		drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		drawImageWrite.dstBinding = 0;
		drawImageWrite.dstSet = m_DrawImageDescriptors;
		drawImageWrite.descriptorCount = 1;
		drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		drawImageWrite.pImageInfo = &imgInfo;

		vkUpdateDescriptorSets(m_Device, 1, &drawImageWrite, 0, nullptr);

	
		m_MainDeletionQueue.PushFunction([&]()
			{
				m_GlobalDescriptorAllocator.DestroyPool(m_Device);
				vkDestroyDescriptorSetLayout(m_Device, m_DrawImageDescriptorLayout, nullptr);
			});

		return true;
	}

	bool VulkanRHI::InitPipelines()
	{
		return InitComputePipeline();
	}

	bool VulkanRHI::InitComputePipeline()
	{
		VkPipelineLayoutCreateInfo computeLayout{};
		computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computeLayout.pNext = nullptr;
		computeLayout.pSetLayouts = &m_DrawImageDescriptorLayout;
		computeLayout.setLayoutCount = 1;

		VK_ASSERT(vkCreatePipelineLayout(m_Device, &computeLayout, nullptr, &m_ComputeLayout));

		VkShaderModule computeDrawShader;
		if (!LoadShaderModule("Shaders/Vulkan/gradient.comp.spv", m_Device, &computeDrawShader))
		{
			VK_ERROR("Failed to load shader module");
			CORE_ASSERT(false);
		}

		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pNext = nullptr;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module = computeDrawShader;
		stageInfo.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = m_ComputeLayout;
		computePipelineCreateInfo.stage = stageInfo;

		VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_ComputePipeline));

		vkDestroyShaderModule(m_Device, computeDrawShader, nullptr);

		m_MainDeletionQueue.PushFunction([&]()
			{
				vkDestroyPipelineLayout(m_Device, m_ComputeLayout, nullptr);
				vkDestroyPipeline(m_Device, m_ComputePipeline, nullptr);
			});

		return true;
	}

	bool VulkanRHI::CreateSwapchain(u32 width, u32 height)
	{
		CORE_ASSERT(width != 0 && height != 0);
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };
		m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		const Window::WindowData& data = Application::Get().GetWindow().GetWindowData();
		const VkPresentModeKHR presentMode = data.Vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
		auto swapchainReturn = swapchainBuilder
			.set_desired_format({ .format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(presentMode)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build();
		vkb::Swapchain swapchain = swapchainReturn.value();
		m_SwapchainExtent = swapchain.extent;
		m_Swapchain = swapchain.swapchain;
		m_SwapchainImages = swapchain.get_images().value();
		m_SwapchainImageViews = swapchain.get_image_views().value();
		return VK_CHECK(swapchainReturn.vk_result());
	}

	void VulkanRHI::DestroySwapchain()
	{
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		for (auto& image : m_SwapchainImageViews)
		{
			vkDestroyImageView(m_Device, image, nullptr);
		}
	}

	void VulkanRHI::ClearBackBuffer(VkCommandBuffer cmd)
	{
		/*const f32 flash = std::abs(std::sin(m_FrameNumber / 120.0f));
		VkClearColorValue clearValue = { {0.0f, 0.0f, flash, 1.0f} };

		VkImageSubresourceRange clearRange = Utils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

		vkCmdClearColorImage(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);*/

		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);

		// bind the descriptor set containing the draw image for the compute pipeline
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputeLayout, 0, 1, &m_DrawImageDescriptors, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, std::ceil(m_DrawExtent.width / 16.0), std::ceil(m_DrawExtent.height / 16.0), 1);
	}
}
