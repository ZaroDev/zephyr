#pragma once
#include <functional>
#include <Zephyr/Renderer/GraphicsAPI.h>
#include <nvrhi/nvrhi.h>

#include <nvrhi/vulkan.h>
#include <GLFW/glfw3.h>
#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <dxgi.h>
#include <d3d11.h>
#include <d3d12.h>
#endif // PLATFORM_WINDOWS

#include <GLFW/glfw3native.h>

namespace Zephyr
{
	class IRenderPass;

    struct DefaultMessageCallback : public nvrhi::IMessageCallback
    {
        static DefaultMessageCallback& GetInstance();
        void message(nvrhi::MessageSeverity severity, const char* messageText) override;
    };

    struct InstanceParameters
    {
        bool EnableDebugRuntime = false;
        bool EnableWarningsAsErrors = false;
        bool EnableGPUValidation = false; // Affects only DX12
        bool HeadlessDevice = false;
        bool LogBufferLifetime = false;
        bool EnableHeapDirectlyIndexed = false; // Allows ResourceDescriptorHeap on DX12

        // Enables per-monitor DPI scale support.
        //
        // If set to true, the app will receive DisplayScaleChanged() events on DPI change and can read
        // the scaling factors using GetDPIScaleInfo(...). The window may be resized when DPI changes if
        // DeviceCreationParameters::resizeWindowWithDisplayScale is true.
        //
        // If set to false, the app will see DPI scaling factors being 1.0 all the time, but the OS
        // may scale the contents of the window based on DPI.
        //
        // This field is located in InstanceParameters and not DeviceCreationParameters because it is needed
        // in the CreateInstance() function to override the glfwInit() behavior.
        bool EnablePerMonitorDPI = false;

    

        // Allows overriding the Vulkan library name with something custom, useful for Streamline
        std::string VulkanLibraryName;

        std::vector<std::string> RequiredVulkanInstanceExtensions;
        std::vector<std::string> RequiredVulkanLayers;
        std::vector<std::string> OptionalVulkanInstanceExtensions;
        std::vector<std::string> OptionalVulkanLayers;
    };

    struct AdapterInfo
    {
        typedef std::array<uint8_t, 16> UUID;
        typedef std::array<uint8_t, 8> LUID;

        std::string Name;
        uint32_t VendorID = 0;
        uint32_t DeviceID = 0;
        uint64_t DedicatedVideoMemory = 0;

        std::optional<UUID> Uuid;
        std::optional<LUID> Luid;

        nvrhi::RefCountPtr<IDXGIAdapter> dxgiAdapter;
        VkPhysicalDevice vkPhysicalDevice = nullptr;
    };

    struct DeviceCreationParameters : public InstanceParameters
    {
        bool StartMaximized = false; // ignores backbuffer width/height to be monitor size
        bool StartFullscreen = false;
        bool StartBorderless = false;
        bool AllowModeSwitch = false;
        i32 WindowPosX = -1;            // -1 means use default placement
        i32 WindowPosY = -1;
        u32 BackBufferWidth = 1280;
        u32 BackBufferHeight = 720;
        u32 RefreshRate = 0;
        u32 SwapChainBufferCount = 3;
        nvrhi::Format SwapChainFormat = nvrhi::Format::SRGBA8_UNORM;
        u32 SwapChainSampleCount = 1;
        u32 SwapChainSampleQuality = 0;
        u32 MaxFramesInFlight = 2;
        bool EnableNvrhiValidationLayer = false;
        bool VsyncEnabled = false;
        bool EnableRayTracingExtensions = false; // for vulkan
        bool EnableComputeQueue = false;
        bool EnableCopyQueue = false;

        // Index of the adapter (DX11, DX12) or physical device (Vk) on which to initialize the device.
        // Negative values mean automatic detection.
        // The order of indices matches that returned by DeviceManager::EnumerateAdapters.
        int AdapterIndex = -1;

        // Set this to true if the application implements UI scaling for DPI explicitly instead of relying
        // on ImGUI's DisplayFramebufferScale. This produces crisp text and lines at any scale
        // but requires considerable changes to applications that rely on the old behavior:
        // all UI sizes and offsets need to be computed as multiples of some scaled parameter,
        // such as ImGui::GetFontSize(). Note that the ImGUI style is automatically reset and scaled in 
        // ImGui_Renderer::DisplayScaleChanged(...).
        //
        // See ImGUI FAQ for more info:
        //   https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-how-should-i-handle-dpi-in-my-application
        bool SupportExplicitDisplayScaling = false;

        // Enables automatic resizing of the application window according to the DPI scaling of the monitor
        // that it is located on. When set to true and the app launches on a monitor with >100% scale, 
        // the initial window size will be larger than specified in 'backBufferWidth' and 'backBufferHeight' parameters.
        bool ResizeWindowWithDisplayScale = false;

        nvrhi::IMessageCallback* MessageCallback = nullptr;

        std::vector<std::string> RequiredVulkanDeviceExtensions;
        std::vector<std::string> OptionalVulkanDeviceExtensions;
        std::vector<size_t> IgnoredVulkanValidationMessageLocations = {
            // Ignore the warnings like "the storage image descriptor [...] is accessed by a OpTypeImage that has
            //   a Format operand ... which doesn't match the VkImageView ..." -- even when the GPU supports
            // storage without format, which all modern GPUs do, there is no good way to enable it in the shaders.
            0x13365b2
        };
        std::function<void(VkDeviceCreateInfo&)> DeviceCreateInfoCallback;
        void* PhysicalDeviceFeatures2Extensions = nullptr;
    };

	class DeviceManager
	{
	public:
		static DeviceManager* Create(GraphicsAPI api);

        bool CreateHeadlessDevice(const DeviceCreationParameters& params);
        bool CreateWindowDeviceAndSwapChain(const DeviceCreationParameters& params, const char* windowTitle);
        // Initializes device-independent objects (DXGI factory, Vulkan instnace).
        // Calling CreateInstance() is required before EnumerateAdapters(), but optional if you don't use EnumerateAdapters().
        // Note: if you call CreateInstance before Create*Device*(), the values in InstanceParameters must match those
        // in DeviceCreationParameters passed to the device call.
        bool CreateInstance(const InstanceParameters& params);

        // Enumerates adapters or physical devices present in the system.
        // Note: a call to CreateInstance() or Create*Device*() is required before EnumerateAdapters().
        virtual bool EnumerateAdapters(std::vector<AdapterInfo>& outAdapters) = 0;

        void AddRenderPassToFront(IRenderPass* pController);
        void AddRenderPassToBack(IRenderPass* pController);
        void RemoveRenderPass(IRenderPass* pController);

        void Update();

        // returns the size of the window in screen coordinates
        void GetWindowDimensions(int& width, int& height) const;
        // returns the screen coordinate to pixel coordinate scale factor
        void GetDPIScaleInfo(float& x, float& y) const
        {
            x = m_DPIScaleFactorX;
            y = m_DPIScaleFactorY;
        }
    protected:
        // useful for apps that require 2 frames worth of simulation data before first render
        // apps should extend the DeviceManager classes, and constructor initialized this to true to opt in to the behavior
        bool m_SkipRenderOnFirstFrame = false;
        bool m_windowVisible = false;
        bool m_windowIsInFocus = true;

        DeviceCreationParameters m_DeviceParams;
        GLFWwindow* m_Window = nullptr;
        bool m_EnableRenderDuringWindowMovement = false;

        // set to true if running on NV GPU
        std::list<IRenderPass*> m_vRenderPasses;
        double m_PreviousFrameTimestamp = 0.0;
        // current DPI scale info (updated when window moves)
        float m_DPIScaleFactorX = 1.f;
        float m_DPIScaleFactorY = 1.f;
        float m_PrevDPIScaleFactorX = 0.f;
        float m_PrevDPIScaleFactorY = 0.f;
        bool m_RequestedVSync = false;
        bool m_InstanceCreated = false;

        double m_AverageFrameTime = 0.0;
        double m_AverageTimeUpdateInterval = 0.5;
        double m_FrameTimeSum = 0.0;
        int m_NumberOfAccumulatedFrames = 0;

        uint32_t m_FrameIndex = 0;

        std::vector<nvrhi::FramebufferHandle> m_SwapChainFramebuffers;

        DeviceManager();

        void UpdateWindowSize();
        bool ShouldRenderUnfocused() const;

        void BackBufferResizing();
        void BackBufferResized();
        void DisplayScaleChanged();

        void Animate(double elapsedTime);
        void Render();
        void UpdateAverageFrameTime(double elapsedTime);
        bool AnimateRenderPresent();
        // device-specific methods
        virtual bool CreateInstanceInternal() = 0;
        virtual bool CreateDevice() = 0;
        virtual bool CreateSwapChain() = 0;
        virtual void DestroyDeviceAndSwapChain() = 0;
        virtual void ResizeSwapChain() = 0;
        virtual bool BeginFrame() = 0;
        virtual bool Present() = 0;

    public:
        NODISCARD virtual nvrhi::IDevice* GetDevice() const = 0;
        NODISCARD virtual const char* GetRendererString() const = 0;
        NODISCARD virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;

        const DeviceCreationParameters& GetDeviceParams();
        NODISCARD double GetAverageFrameTimeSeconds() const { return m_AverageFrameTime; }
        NODISCARD double GetPreviousFrameTimestamp() const { return m_PreviousFrameTimestamp; }
        void SetFrameTimeUpdateInterval(double seconds) { m_AverageTimeUpdateInterval = seconds; }
		NODISCARD bool IsVsyncEnabled() const { return m_DeviceParams.VsyncEnabled; }
        virtual void SetVsyncEnabled(bool enabled) { m_RequestedVSync = enabled; /* will be processed later */ }
        virtual void ReportLiveObjects() {}
        void SetEnableRenderDuringWindowMovement(bool val) { m_EnableRenderDuringWindowMovement = val; }

        // these are public in order to be called from the GLFW callback functions
        void WindowPosCallback(int xpos, int ypos);

        NODISCARD GLFWwindow* GetWindow() const { return m_Window; }
        NODISCARD uint32_t GetFrameIndex() const { return m_FrameIndex; }

        virtual nvrhi::ITexture* GetCurrentBackBuffer() = 0;
        virtual nvrhi::ITexture* GetBackBuffer(uint32_t index) = 0;
        virtual uint32_t GetCurrentBackBufferIndex() = 0;
        virtual uint32_t GetBackBufferCount() = 0;
        nvrhi::IFramebuffer* GetCurrentFramebuffer();
        nvrhi::IFramebuffer* GetFramebuffer(uint32_t index);

        virtual void Shutdown();
        virtual ~DeviceManager() = default;

        void SetWindowTitle(const char* title);
        void SetInformativeWindowTitle(const char* applicationName, bool includeFramerate = true, const char* extraInfo = nullptr);
        const char* GetWindowTitle();

        virtual bool IsVulkanInstanceExtensionEnabled(const char* extensionName) const { return false; }
        virtual bool IsVulkanDeviceExtensionEnabled(const char* extensionName) const { return false; }
        virtual bool IsVulkanLayerEnabled(const char* layerName) const { return false; }
        virtual void GetEnabledVulkanInstanceExtensions(std::vector<std::string>& extensions) const {}
        virtual void GetEnabledVulkanDeviceExtensions(std::vector<std::string>& extensions) const {}
        virtual void GetEnabledVulkanLayers(std::vector<std::string>& layers) const {}

        // GetFrameIndex cannot be used inside of these callbacks, hence the additional passing of frameID
        // Refer to AnimateRenderPresent implementation for more details
        struct PipelineCallbacks
		{
            std::function<void(DeviceManager&, uint32_t)> BeforeFrame = nullptr;
            std::function<void(DeviceManager&, uint32_t)> BeforeAnimate = nullptr;
            std::function<void(DeviceManager&, uint32_t)> AfterAnimate = nullptr;
            std::function<void(DeviceManager&, uint32_t)> BeforeRender = nullptr;
            std::function<void(DeviceManager&, uint32_t)> AfterRender = nullptr;
            std::function<void(DeviceManager&, uint32_t)> BeforePresent = nullptr;
            std::function<void(DeviceManager&, uint32_t)> AfterPresent = nullptr;
        } m_Callbacks;
    private:
        static DeviceManager* CreateD3D11();
        static DeviceManager* CreateD3D12();
        static DeviceManager* CreateVK();

        String m_WindowTitle;
	};
}
