#pragma once


#include <Zephyr/Renderer/RenderHardwareInterface.h>

#include <vk-bootstrap/VkBootstrap.h>

#include "DeletionQueue.h"
#include "VulkanDescriptors.h"

#ifdef PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vma/vk_mem_alloc.h>

namespace Zephyr
{
	struct AllocatedImage
	{
		VkImage Image;
		VkImageView ImageView;
		VmaAllocation Allocation;
		VkExtent3D ImageExtent;
		VkFormat ImageFormat;
	};

	struct FrameData
	{
		VkCommandPool CommandPool;
		VkCommandBuffer MainCommandBuffer;
		VkSemaphore SwapChainSemaphore;
		VkSemaphore RenderSemaphore;
		VkFence RenderFence;

		DeletionQueue DeletionQueue;
	};

	constexpr u32 c_FrameOverlap = 3;

	class VulkanRHI final : public RenderHardwareInterface
	{
	public:
		VulkanRHI() = default;
		~VulkanRHI() = default;

		bool Init() override;
		void Shutdown() override;
		NODISCARD const char* GetName() const override { return "Vulkan"; }
		NODISCARD RenderDevice GetRenderDevice() const override { return m_RenderDeviceInfo; }
		void OnResize(u32 width, u32 height) override;
		void BeginFrame() override;
		void EndFrame() override;

		FrameData& GetCurrentFrame() { return m_Frames[m_FrameNumber % c_FrameOverlap]; }


		virtual bool ImGuiInit() override;
		virtual void ImGuiNewFrame() override;
		virtual void ImGuiEndFrame() override;
		virtual void ImGuiShutdown() override;
		virtual void RebuildFontTextures() const override;
	private:

		bool InitVulkan();
		bool InitSwapChain();
		bool InitCommands();
		bool InitSyncStructures();
		bool InitDescriptors();
		bool InitPipelines();

		bool InitComputePipeline();

		bool CreateSwapchain(u32 width, u32 height);
		void DestroySwapchain();

		void ClearBackBuffer(VkCommandBuffer cmd);

	private:
		RenderDevice m_RenderDeviceInfo;

		VkInstance m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;
		VkDebugUtilsMessengerEXT m_DebugMessenger;

		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkExtent2D m_SwapchainExtent;
		u32 m_CurrentSwapchainImage = 0;

		AllocatedImage m_DrawImage;
		VkExtent2D m_DrawExtent;


		FrameData m_Frames[c_FrameOverlap]{};
		u32 m_FrameNumber = 0;
		VkQueue m_GraphicsQueue;
		u32 m_GraphicsQueueFamily;

		DeletionQueue m_MainDeletionQueue;

		VmaAllocator m_Allocator;

		DescriptorAllocator m_GlobalDescriptorAllocator;
		VkDescriptorSet m_DrawImageDescriptors;
		VkDescriptorSetLayout m_DrawImageDescriptorLayout;

		VkDescriptorPool m_ImGuiPool;

		// Compute
		VkPipeline m_ComputePipeline;
		VkPipelineLayout m_ComputeLayout;
	};
}
