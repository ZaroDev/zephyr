#include <pch.h>
#include "DeviceManager.h"

#include "IRenderPass.h"
#include "nvrhi/utils.h"
#include "Platform/Vulkan/VulkanDeviceManager.h"

#ifdef PLATFORM_WINDOWS
#include <ShellScalingApi.h>
#pragma comment(lib, "shcore.lib")
#endif

namespace Zephyr
{
#if defined(PLATFORM_WINDOWS)
	extern "C"
	{
		// Declaring this symbol makes the OS run the app on the discrete GPU on NVIDIA Optimus laptops by default
		__declspec(dllexport) DWORD NvOptimusEnablement = 1;
		// Same as above, for laptops with AMD GPUs
		__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;
	}
#endif


	static const struct
	{
		nvrhi::Format Format;
		u32 RedBits;
		u32 GreenBits;
		u32 BlueBits;
		u32 AlphaBits;
		u32 DepthBits;
		u32 StencilBits;
	} c_FormatInfo[] = {
		{ nvrhi::Format::UNKNOWN,            0,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::R8_UINT,            8,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::RG8_UINT,           8,  8,  0,  0,  0,  0, },
		{ nvrhi::Format::RG8_UNORM,          8,  8,  0,  0,  0,  0, },
		{ nvrhi::Format::R16_UINT,          16,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::R16_UNORM,         16,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::R16_FLOAT,         16,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::RGBA8_UNORM,        8,  8,  8,  8,  0,  0, },
		{ nvrhi::Format::RGBA8_SNORM,        8,  8,  8,  8,  0,  0, },
		{ nvrhi::Format::BGRA8_UNORM,        8,  8,  8,  8,  0,  0, },
		{ nvrhi::Format::SRGBA8_UNORM,       8,  8,  8,  8,  0,  0, },
		{ nvrhi::Format::SBGRA8_UNORM,       8,  8,  8,  8,  0,  0, },
		{ nvrhi::Format::R10G10B10A2_UNORM, 10, 10, 10,  2,  0,  0, },
		{ nvrhi::Format::R11G11B10_FLOAT,   11, 11, 10,  0,  0,  0, },
		{ nvrhi::Format::RG16_UINT,         16, 16,  0,  0,  0,  0, },
		{ nvrhi::Format::RG16_FLOAT,        16, 16,  0,  0,  0,  0, },
		{ nvrhi::Format::R32_UINT,          32,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::R32_FLOAT,         32,  0,  0,  0,  0,  0, },
		{ nvrhi::Format::RGBA16_FLOAT,      16, 16, 16, 16,  0,  0, },
		{ nvrhi::Format::RGBA16_UNORM,      16, 16, 16, 16,  0,  0, },
		{ nvrhi::Format::RGBA16_SNORM,      16, 16, 16, 16,  0,  0, },
		{ nvrhi::Format::RG32_UINT,         32, 32,  0,  0,  0,  0, },
		{ nvrhi::Format::RG32_FLOAT,        32, 32,  0,  0,  0,  0, },
		{ nvrhi::Format::RGB32_UINT,        32, 32, 32,  0,  0,  0, },
		{ nvrhi::Format::RGB32_FLOAT,       32, 32, 32,  0,  0,  0, },
		{ nvrhi::Format::RGBA32_UINT,       32, 32, 32, 32,  0,  0, },
		{ nvrhi::Format::RGBA32_FLOAT,      32, 32, 32, 32,  0,  0, },
	};

	static void GLWFErrorCallback(int error, const char* description)
	{
		CORE_ERROR("[GLFW] Error {0}: {1}", error, description);
	}

	DeviceManager* DeviceManager::Create(GraphicsAPI api)
	{
		switch (api)
		{
		case GraphicsAPI::D3D11:
			return CreateD3D11();
		case GraphicsAPI::D3D12:
			return CreateD3D12();
		case GraphicsAPI::VULKAN:
			return CreateVK();
		default:
			CORE_ASSERT(false);
		}

		return nullptr;
	}

	bool DeviceManager::CreateHeadlessDevice(const DeviceCreationParameters& params)
	{
		m_DeviceParams = params;
		m_DeviceParams.HeadlessDevice = true;

		if (!CreateInstance(m_DeviceParams))
		{
			return false;
		}

		return CreateDevice();
	}

	bool DeviceManager::CreateWindowDeviceAndSwapChain(const DeviceCreationParameters& params, const char* windowTitle)
	{
		m_DeviceParams = params;
		m_DeviceParams.HeadlessDevice = false;
		m_RequestedVSync = params.VsyncEnabled;

		if (!CreateInstance(m_DeviceParams))
		{
			return false;
		}

		glfwSetErrorCallback(GLWFErrorCallback);
		glfwDefaultWindowHints();

		bool foundFormat = false;
		for (const auto& info : c_FormatInfo)
		{
			if (info.Format == params.SwapChainFormat)
			{
				glfwWindowHint(GLFW_RED_BITS, info.RedBits);
				glfwWindowHint(GLFW_GREEN_BITS, info.GreenBits);
				glfwWindowHint(GLFW_BLUE_BITS, info.BlueBits);
				glfwWindowHint(GLFW_ALPHA_BITS, info.AlphaBits);
				glfwWindowHint(GLFW_DEPTH_BITS, info.DepthBits);
				glfwWindowHint(GLFW_STENCIL_BITS, info.StencilBits);
				foundFormat = true;
				break;
			}
		}

		CORE_ASSERT(foundFormat, "Failed to find window format");

		glfwWindowHint(GLFW_SAMPLES, params.SwapChainSampleCount);
		glfwWindowHint(GLFW_REFRESH_RATE, params.RefreshRate);
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, params.ResizeWindowWithDisplayScale);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		if (params.StartBorderless)
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		}
		m_Window = glfwCreateWindow(params.BackBufferWidth, params.BackBufferHeight, windowTitle ? windowTitle : "Zephyr Window", params.StartFullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);


		if (m_Window == nullptr)
		{
			return false;
		}

		if (params.StartFullscreen)
		{
			glfwSetWindowMonitor(m_Window, glfwGetPrimaryMonitor(), 0, 0, m_DeviceParams.BackBufferWidth, m_DeviceParams.BackBufferHeight, m_DeviceParams.RefreshRate);
		}
		else
		{
			i32 fbWith = 0, fbHeight;
			glfwGetFramebufferSize(m_Window, &fbWith, &fbHeight);
			m_DeviceParams.BackBufferWidth = fbWith;
			m_DeviceParams.BackBufferHeight = fbHeight;
		}


		if (windowTitle)
		{
			m_WindowTitle = windowTitle;
		}

		glfwSetWindowUserPointer(m_Window, this);

		if (params.WindowPosX != -1 && params.WindowPosY != -1)
		{
			glfwSetWindowPos(m_Window, params.WindowPosX, params.WindowPosY);
		}

		/*glfwSetWindowPosCallback(m_Window, WindowPosCallback_GLFW);
		glfwSetWindowCloseCallback(m_Window, WindowCloseCallback_GLFW);
		glfwSetWindowRefreshCallback(m_Window, WindowRefreshCallback_GLFW);
		glfwSetWindowFocusCallback(m_Window, WindowFocusCallback_GLFW);
		glfwSetWindowIconifyCallback(m_Window, WindowIconifyCallback_GLFW);
		glfwSetKeyCallback(m_Window, KeyCallback_GLFW);
		glfwSetCharModsCallback(m_Window, CharModsCallback_GLFW);
		glfwSetCursorPosCallback(m_Window, MousePosCallback_GLFW);
		glfwSetMouseButtonCallback(m_Window, MouseButtonCallback_GLFW);
		glfwSetScrollCallback(m_Window, MouseScrollCallback_GLFW);*/
		
		if (!CreateDevice())
			return false;

		if (!CreateSwapChain())
			return false;

		glfwShowWindow(m_Window);

		if (m_DeviceParams.StartMaximized)
		{
			glfwMaximizeWindow(m_Window);
		}

		// reset the back buffer size state to enforce a resize event
		m_DeviceParams.BackBufferWidth = 0;
		m_DeviceParams.BackBufferHeight = 0;

		UpdateWindowSize();

		return true;
	}

	bool DeviceManager::CreateInstance(const InstanceParameters& params)
	{
		if (m_InstanceCreated)
		{
			return false;
		}


		static_cast<InstanceParameters&>(m_DeviceParams) = params;
		if (!params.HeadlessDevice)
		{
#ifdef PLATFORM_WINDOWS
			if (!params.EnablePerMonitorDPI)
			{
				// glfwInit enables the maximum supported level of DPI awareness unconditionally.
				// If the app doesn't need it, we have to call this function before glfwInit to override that behavior.
				SetProcessDpiAwareness(PROCESS_DPI_UNAWARE);
			}
#endif
			if (!glfwInit())
			{
				return false;
			}
		}

		m_InstanceCreated = CreateInstanceInternal();
		return m_InstanceCreated;
	}

	void DeviceManager::AddRenderPassToFront(IRenderPass* pController)
	{
		m_vRenderPasses.remove(pController);
		m_vRenderPasses.push_front(pController);

		pController->BackBufferResizing();
		pController->BackBufferResized(
			m_DeviceParams.BackBufferWidth,
			m_DeviceParams.BackBufferHeight,
			m_DeviceParams.SwapChainSampleCount);
	}

	void DeviceManager::AddRenderPassToBack(IRenderPass* pController)
	{
		m_vRenderPasses.remove(pController);
		m_vRenderPasses.push_back(pController);

		pController->BackBufferResizing();
		pController->BackBufferResized(
			m_DeviceParams.BackBufferWidth,
			m_DeviceParams.BackBufferHeight,
			m_DeviceParams.SwapChainSampleCount);
	}

	void DeviceManager::RemoveRenderPass(IRenderPass* pController)
	{
		m_vRenderPasses.remove(pController);
	}

	void DeviceManager::Update()
	{
		m_PreviousFrameTimestamp = glfwGetTime();

		while (!glfwWindowShouldClose(m_Window))
		{

			if (m_Callbacks.beforeFrame) m_Callbacks.beforeFrame(*this, m_FrameIndex);
			glfwPollEvents();
			UpdateWindowSize();
			bool presentSuccess = AnimateRenderPresent();
			if (!presentSuccess)
			{
				break;
			}
		}

		bool waitSuccess = GetDevice()->waitForIdle();
	}

	void DeviceManager::GetWindowDimensions(int& width, int& height) const
	{
		width = m_DeviceParams.BackBufferWidth;
		height = m_DeviceParams.BackBufferHeight;
	}

	DeviceManager::DeviceManager()
	{
	}

	void DeviceManager::UpdateWindowSize()
	{
		i32 width;
		i32 height;
		glfwGetWindowSize(m_Window, &width, &height);

		if (width == 0 || height == 0)
		{
			// window is minimized
			m_windowVisible = false;
			return;
		}

		m_windowVisible = true;

		m_windowIsInFocus = glfwGetWindowAttrib(m_Window, GLFW_FOCUSED) == 1;

		if (i32(m_DeviceParams.BackBufferWidth) != width ||
			i32(m_DeviceParams.BackBufferHeight) != height ||
			(m_DeviceParams.VsyncEnabled != m_RequestedVSync && GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN))
		{
			// window is not minimized, and the size has changed

			BackBufferResizing();

			m_DeviceParams.BackBufferWidth = width;
			m_DeviceParams.BackBufferHeight = height;
			m_DeviceParams.VsyncEnabled = m_RequestedVSync;

			ResizeSwapChain();
			BackBufferResized();
		}

		m_DeviceParams.VsyncEnabled = m_RequestedVSync;
	}

	bool DeviceManager::ShouldRenderUnfocused() const
	{
		for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
		{
			bool ret = (*it)->ShouldRenderUnfocused();
			if (ret)
				return true;
		}

		return false;
	}

	void DeviceManager::BackBufferResizing()
	{
		m_SwapChainFramebuffers.clear();

		for (auto it : m_vRenderPasses)
		{
			it->BackBufferResizing();
		}
	}

	void DeviceManager::BackBufferResized()
	{
		for (auto it : m_vRenderPasses)
		{
			it->BackBufferResized(m_DeviceParams.BackBufferWidth,
				m_DeviceParams.BackBufferHeight,
				m_DeviceParams.SwapChainSampleCount);
		}

		u32 backBufferCount = GetBackBufferCount();
		m_SwapChainFramebuffers.resize(backBufferCount);
		for (u32 index = 0; index < backBufferCount; index++)
		{
			m_SwapChainFramebuffers[index] = GetDevice()->createFramebuffer(
				nvrhi::FramebufferDesc().addColorAttachment(GetBackBuffer(index)));
		}
	}

	void DeviceManager::DisplayScaleChanged()
	{
		for (auto it : m_vRenderPasses)
		{
			it->DisplayScaleChanged(m_DPIScaleFactorX, m_DPIScaleFactorY);
		}
	}

	void DeviceManager::Animate(double elapsedTime)
	{
		for (auto it : m_vRenderPasses)
		{
			it->Animate(float(elapsedTime));
			it->SetLatewarpOptions();
		}
	}

	void DeviceManager::Render()
	{
		nvrhi::IFramebuffer* framebuffer = m_SwapChainFramebuffers[GetCurrentBackBufferIndex()];

		for (auto it : m_vRenderPasses)
		{
			it->Render(framebuffer);
		}
	}

	void DeviceManager::UpdateAverageFrameTime(double elapsedTime)
	{
		m_FrameTimeSum += elapsedTime;
		m_NumberOfAccumulatedFrames += 1;

		if (m_FrameTimeSum > m_AverageTimeUpdateInterval && m_NumberOfAccumulatedFrames > 0)
		{
			m_AverageFrameTime = m_FrameTimeSum / double(m_NumberOfAccumulatedFrames);
			m_NumberOfAccumulatedFrames = 0;
			m_FrameTimeSum = 0.0;
		}
	}

	bool DeviceManager::AnimateRenderPresent()
	{
		double curTime = glfwGetTime();
		double elapsedTime = curTime - m_PreviousFrameTimestamp;

		if (m_windowVisible && (m_windowIsInFocus || ShouldRenderUnfocused()))
		{
			if (m_PrevDPIScaleFactorX != m_DPIScaleFactorX || m_PrevDPIScaleFactorY != m_DPIScaleFactorY)
			{
				DisplayScaleChanged();
				m_PrevDPIScaleFactorX = m_DPIScaleFactorX;
				m_PrevDPIScaleFactorY = m_DPIScaleFactorY;
			}

			if (m_Callbacks.beforeAnimate) m_Callbacks.beforeAnimate(*this, m_FrameIndex);
			Animate(elapsedTime);
			if (m_Callbacks.afterAnimate) m_Callbacks.afterAnimate(*this, m_FrameIndex);

			// normal rendering           : A0    R0 P0 A1 R1 P1
			// m_SkipRenderOnFirstFrame on: A0 A1 R0 P0 A2 R1 P1
			// m_SkipRenderOnFirstFrame simulates multi-threaded rendering frame indices, m_FrameIndex becomes the simulation index
			// while the local variable below becomes the render/present index, which will be different only if m_SkipRenderOnFirstFrame is set
			if (m_FrameIndex > 0 || !m_SkipRenderOnFirstFrame)
			{
				if (BeginFrame())
				{
					// first time entering this loop, m_FrameIndex is 1 for m_SkipRenderOnFirstFrame, 0 otherwise;
					uint32_t frameIndex = m_FrameIndex;

					if (m_SkipRenderOnFirstFrame)
					{
						frameIndex--;
					}

					if (m_Callbacks.beforeRender) m_Callbacks.beforeRender(*this, frameIndex);
					Render();
					if (m_Callbacks.afterRender) m_Callbacks.afterRender(*this, frameIndex);
					if (m_Callbacks.beforePresent) m_Callbacks.beforePresent(*this, frameIndex);
					bool presentSuccess = Present();
					if (m_Callbacks.afterPresent) m_Callbacks.afterPresent(*this, frameIndex);
					if (!presentSuccess)
					{
						return false;
					}
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(0));

		GetDevice()->runGarbageCollection();

		UpdateAverageFrameTime(elapsedTime);
		m_PreviousFrameTimestamp = curTime;

		++m_FrameIndex;
		return true;
	}
	const DeviceCreationParameters& DeviceManager::GetDeviceParams()
	{
		return m_DeviceParams;
	}

	void DeviceManager::WindowPosCallback(int xpos, int ypos)
	{
		if (m_DeviceParams.EnablePerMonitorDPI)
		{
#ifdef PLATFORM_WINDOWS
			// Use Windows-specific implementation of DPI query because GLFW has issues:
			// glfwGetWindowMonitor(window) returns NULL for non-fullscreen applications.
			// This custom code allows us to adjust DPI scaling when a window is moved
			// between monitors with different scales.

			HWND hwnd = glfwGetWin32Window(m_Window);
			auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

			unsigned int dpiX;
			unsigned int dpiY;
			GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

			m_DPIScaleFactorX = dpiX / 96.f;
			m_DPIScaleFactorY = dpiY / 96.f;
#else
			// Linux support for display scaling using GLFW.
			// This has limited utility due to the issue described above (NULL monitor),
			// and because GLFW doesn't support fractional scaling properly.
			// For example, on a system with 150% scaling it will report scale = 2.0
			// but the window will be either too small or too big, depending on 'resizeWindowWithDisplayScale'

			GLFWmonitor* monitor = glfwGetWindowMonitor(m_Window);

			// Non-fullscreen windows have NULL monitor, let's use the primary monitor in this case
			if (!monitor)
				monitor = glfwGetPrimaryMonitor();

			glfwGetMonitorContentScale(monitor, &m_DPIScaleFactorX, &m_DPIScaleFactorY);
#endif
		}

		if (m_EnableRenderDuringWindowMovement && m_SwapChainFramebuffers.size() > 0)
		{
			if (m_Callbacks.beforeFrame)
			{
				m_Callbacks.beforeFrame(*this, m_FrameIndex);
			}
			AnimateRenderPresent();
		}
	}

	void DeviceManager::SetWindowTitle(const char* title)
	{
		CORE_ASSERT(title);
		if (m_WindowTitle == title)
			return;

		glfwSetWindowTitle(m_Window, title);

		m_WindowTitle = title;
	}

	void DeviceManager::SetInformativeWindowTitle(const char* applicationName, bool includeFramerate,
		const char* extraInfo)
	{
		std::stringstream ss;
		ss << applicationName;
		ss << " (" << nvrhi::utils::GraphicsAPIToString(GetDevice()->getGraphicsAPI());

		if (m_DeviceParams.EnableDebugRuntime)
		{
			if (GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN)
				ss << ", VulkanValidationLayer";
			else
				ss << ", DebugRuntime";
		}

		if (m_DeviceParams.EnableNvrhiValidationLayer)
		{
			ss << ", NvrhiValidationLayer";
		}

		ss << ")";

		double frameTime = GetAverageFrameTimeSeconds();
		if (includeFramerate && frameTime > 0)
		{
			double const fps = 1.0 / frameTime;
			int const precision = (fps <= 20.0) ? 1 : 0;
			ss << " - " << std::fixed << std::setprecision(precision) << fps << " FPS ";
		}

		if (extraInfo)
			ss << extraInfo;

		SetWindowTitle(ss.str().c_str());
	}

	const char* DeviceManager::GetWindowTitle()
	{
		return m_WindowTitle.c_str();
	}

	nvrhi::IFramebuffer* DeviceManager::GetCurrentFramebuffer()
	{
		return GetFramebuffer(GetCurrentBackBufferIndex());
	}

	nvrhi::IFramebuffer* DeviceManager::GetFramebuffer(uint32_t index)
	{
		if (index < m_SwapChainFramebuffers.size())
			return m_SwapChainFramebuffers[index];

		return nullptr;
	}

	void DeviceManager::Shutdown()
	{
		m_SwapChainFramebuffers.clear();
		DestroyDeviceAndSwapChain();

		m_InstanceCreated = false;
	}

	DeviceManager* DeviceManager::CreateD3D11()
	{
		return nullptr;
	}

	DeviceManager* DeviceManager::CreateD3D12()
	{
		return nullptr;
	}


	DefaultMessageCallback& DefaultMessageCallback::GetInstance()
	{
		static DefaultMessageCallback Instance;
		return Instance;
	}

	void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char* messageText)
	{
		
		switch (severity)
		{
		case nvrhi::MessageSeverity::Info:
			CORE_INFO("[NVRHI]: {}", messageText);
			break;
		case nvrhi::MessageSeverity::Warning:
			CORE_WARN("[NVRHI]: {}", messageText);
			break;
		case nvrhi::MessageSeverity::Error:
			CORE_ERROR("[NVRHI]: {}", messageText);
			break;
		case nvrhi::MessageSeverity::Fatal:
			CORE_CRITICAL("[NVRHI]: {}", messageText);
			break;
		}

	}

}
