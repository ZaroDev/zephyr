#include <pch.h>
#include "Renderer.h"
#include "Core/Application.h"

#include <Zephyr/Time/Time.h>
#include <imgui.h>

#include "Platform/Vulkan/VulkanRHI.h"

namespace Zephyr
{
	bool Renderer::Initialize(GraphicsAPI api)
	{
		m_API = api;
		

		bool ret = true;

		m_GraphicsInterface = CreateRHI(api);
		if(!m_GraphicsInterface)
		{
			CORE_ASSERT(false, "Failed to create RHI");
		}
		ret &= m_GraphicsInterface->Init();

		//ret &= m_Library.LoadEngineShaders();


		ret &= InitImGui();
		ret &= m_GraphicsInterface->ImGuiInit();


		const Window::WindowData& WindowData = Application::Get().GetWindow().GetWindowData();
		const FramebufferSpecification spec = {
			.Width = WindowData.Width,
			.Height = WindowData.Height,
			.Attachments = FramebufferAttachmentSpecification{
				FramebufferTextureFormat::RGBA16,
			},
			.Samples = 1,
			.SwapChainTarget = false,
		};

		m_ViewPort = Framebuffer::Create(spec);


		return ret;
	}

	void Renderer::Shutdown()
	{
		CORE_INFO("Closing renderer!");

		m_ViewPort.reset();

		m_GraphicsInterface->ImGuiShutdown();
		ShutdownImGui();
		m_GraphicsInterface->Shutdown();
	}
	void Renderer::OnResize(u32 width, u32 height)
	{
		CORE_INFO("Resize event: {0}x{1}p", width, height);
		m_GraphicsInterface->OnResize(width, height);
	}

	

	void Renderer::BeginFrame()
	{
		m_GraphicsInterface->BeginFrame();
	}
	void Renderer::EndFrame()
	{
		m_GraphicsInterface->EndFrame();
	}
	void Renderer::ImGuiNewFrame()
	{
		m_GraphicsInterface->ImGuiNewFrame();
		ImGui::NewFrame();
	}
	void Renderer::ImGuiEndFrame()
	{
		ImGui::Render();
		m_GraphicsInterface->ImGuiEndFrame();

		if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void Renderer::RebuildFontTextures() const
	{
		m_GraphicsInterface->RebuildFontTextures();
	}

	Scope<RenderHardwareInterface> Renderer::CreateRHI(GraphicsAPI api)
	{
		switch (api)
		{
		case GraphicsAPI::VULKAN: return CreateScope<VulkanRHI>();
		}

		return nullptr;
	}


	bool Renderer::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		//io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		return true;
	}
	void Renderer::ShutdownImGui()
	{
		ImGui::DestroyContext();
	}
}
