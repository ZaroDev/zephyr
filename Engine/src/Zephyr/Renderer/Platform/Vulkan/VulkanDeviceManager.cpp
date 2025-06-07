#include <pch.h>
#include "VulkanDeviceManager.h"
#include <nvrhi/vulkan.h>
#include <GLFW/glfw3.h>
#include <nvrhi/validation.h>
// Define the Vulkan dynamic dispatcher - this needs to occur in exactly one cpp file in the program.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Zephyr
{
    static constexpr u32 kComputeQueueIndex = 0;
    static constexpr u32 kGraphicsQueueIndex = 0;
    static constexpr u32 kPresentQueueIndex = 0;
    static constexpr u32 kTransferQueueIndex = 0;

    static std::vector<const char*> StringSetToVector(const std::unordered_set<std::string>& set)
    {
        std::vector<const char*> ret;
        for (const auto& s : set)
        {
            ret.push_back(s.c_str());
        }

        return ret;
    }

    template <typename T>
    static std::vector<T> SetToVector(const std::unordered_set<T>& set)
    {
        std::vector<T> ret;
        for (const auto& s : set)
        {
            ret.push_back(s);
        }

        return ret;
    }

    bool VulkanDeviceManager::createInstance()
    {
        if (!m_DeviceParams.HeadlessDevice)
        {
            if (!glfwVulkanSupported())
            {
                CORE_ERROR("GLFW reports that Vulkan is not supported. Perhaps missing a call to glfwInit()?");
                return false;
            }

            // add any extensions required by GLFW
            u32 glfwExtCount;
            const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);
            assert(glfwExt);

            for (u32 i = 0; i < glfwExtCount; i++)
            {
                m_EnabledExtensions.Instance.insert(std::string(glfwExt[i]));
            }
        }

        // add Instance extensions requested by the user
        for (const std::string& name : m_DeviceParams.RequiredVulkanInstanceExtensions)
        {
            m_EnabledExtensions.Instance.insert(name);
        }
        for (const std::string& name : m_DeviceParams.OptionalVulkanInstanceExtensions)
        {
            m_OptionalExtensions.Instance.insert(name);
        }

        // add Layers requested by the user
        for (const std::string& name : m_DeviceParams.RequiredVulkanLayers)
        {
            m_EnabledExtensions.Layers.insert(name);
        }
        for (const std::string& name : m_DeviceParams.OptionalVulkanLayers)
        {
            m_OptionalExtensions.Layers.insert(name);
        }

        std::unordered_set<std::string> requiredExtensions = m_EnabledExtensions.Instance;

        // figure out which optional extensions are supported
        for (const auto& instanceExt : vk::enumerateInstanceExtensionProperties())
        {
            const std::string name = instanceExt.extensionName;
            if (m_OptionalExtensions.Instance.find(name) != m_OptionalExtensions.Instance.end())
            {
                m_EnabledExtensions.Instance.insert(name);
            }

            requiredExtensions.erase(name);
        }

        if (!requiredExtensions.empty())
        {
            std::stringstream ss;
            ss << "Cannot create a Vulkan Instance because the following required extension(s) are not supported:";
            for (const auto& ext : requiredExtensions)
                ss << std::endl << "  - " << ext;

            CORE_ERROR("{}", ss.str().c_str());
            return false;
        }

        CORE_INFO("Enabled Vulkan Instance extensions:");
        for (const auto& ext : m_EnabledExtensions.Instance)
        {
            CORE_INFO("    {}", ext.c_str());
        }

        std::unordered_set<std::string> requiredLayers = m_EnabledExtensions.Layers;

        for (const auto& layer : vk::enumerateInstanceLayerProperties())
        {
            const std::string name = layer.layerName;
            if (m_OptionalExtensions.Layers.find(name) != m_OptionalExtensions.Layers.end())
            {
                m_EnabledExtensions.Layers.insert(name);
            }

            requiredLayers.erase(name);
        }

        if (!requiredLayers.empty())
        {
            std::stringstream ss;
            ss << "Cannot create a Vulkan Instance because the following required layer(s) are not supported:";
            for (const auto& ext : requiredLayers)
                ss << std::endl << "  - " << ext;

            CORE_ERROR("{}", ss.str().c_str());
            return false;
        }

        CORE_INFO("Enabled Vulkan Layers:");
        for (const auto& layer : m_EnabledExtensions.Layers)
        {
            CORE_INFO("    {}", layer.c_str());
        }

        auto instanceExtVec = StringSetToVector(m_EnabledExtensions.Instance);
        auto layerVec = StringSetToVector(m_EnabledExtensions.Layers);

        auto applicationInfo = vk::ApplicationInfo();

        // Query the Vulkan API version supported on the system to make sure we use at least 1.3 when that's present.
        vk::Result res = vk::enumerateInstanceVersion(&applicationInfo.apiVersion);

        if (res != vk::Result::eSuccess)
        {
            CORE_ERROR("Call to vkEnumerateInstanceVersion failed, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
            return false;
        }

        const u32 minimumVulkanVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

        // Check if the Vulkan API version is sufficient.
        if (applicationInfo.apiVersion < minimumVulkanVersion)
        {
            CORE_ERROR("The Vulkan API version supported on the system ({0}.{1}.{2}) is too low, at least {3}.{4}.{5} is required.",
                VK_API_VERSION_MAJOR(applicationInfo.apiVersion), VK_API_VERSION_MINOR(applicationInfo.apiVersion), VK_API_VERSION_PATCH(applicationInfo.apiVersion),
                VK_API_VERSION_MAJOR(minimumVulkanVersion), VK_API_VERSION_MINOR(minimumVulkanVersion), VK_API_VERSION_PATCH(minimumVulkanVersion));
            return false;
        }

        // Spec says: A non-zero variant indicates the API is a variant of the Vulkan API and applications will typically need to be modified to run against it.
        if (VK_API_VERSION_VARIANT(applicationInfo.apiVersion) != 0)
        {
            CORE_ERROR("The Vulkan API supported on the system uses an unexpected variant: {}", VK_API_VERSION_VARIANT(applicationInfo.apiVersion));
            return false;
        }

        // Create the vulkan Instance
        vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
            .setEnabledLayerCount(u32(layerVec.size()))
            .setPpEnabledLayerNames(layerVec.data())
            .setEnabledExtensionCount(u32(instanceExtVec.size()))
            .setPpEnabledExtensionNames(instanceExtVec.data())
            .setPApplicationInfo(&applicationInfo);

        res = vk::createInstance(&info, nullptr, &m_VulkanInstance);
        if (res != vk::Result::eSuccess)
        {
            CORE_ERROR("Failed to create a Vulkan Instance, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
            return false;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanInstance);

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
        vk::DebugReportFlagsEXT flags,
        vk::DebugReportObjectTypeEXT objType,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char* layerPrefix,
        const char* msg,
        void* userData)
    {
        const VulkanDeviceManager* manager = (const VulkanDeviceManager*)userData;

        if (manager)
        {
            const auto& ignored = manager->GetDeviceParams().IgnoredVulkanValidationMessageLocations;
            const auto found = std::find(ignored.begin(), ignored.end(), location);
            if (found != ignored.end())
                return VK_FALSE;
        }

		CORE_WARN("[Vulkan: location=0x{0} code={1}, layerPrefix='{2}'] {3}", location, code, layerPrefix, msg);

        return VK_FALSE;
    }

    void VulkanDeviceManager::installDebugCallback()
    {
        auto info = vk::DebugReportCallbackCreateInfoEXT()
            .setFlags(vk::DebugReportFlagBitsEXT::eError |
                vk::DebugReportFlagBitsEXT::eWarning |
                //   vk::DebugReportFlagBitsEXT::eInformation |
                vk::DebugReportFlagBitsEXT::ePerformanceWarning)
            .setPfnCallback(vulkanDebugCallback)
            .setPUserData(this);

        vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT(&info, nullptr, &m_DebugReportCallback);
        assert(res == vk::Result::eSuccess);
    }

    bool VulkanDeviceManager::pickPhysicalDevice()
    {
        VkFormat requestedFormat = nvrhi::vulkan::convertFormat(m_DeviceParams.SwapChainFormat);
        vk::Extent2D requestedExtent(m_DeviceParams.BackBufferWidth, m_DeviceParams.BackBufferHeight);

        auto devices = m_VulkanInstance.enumeratePhysicalDevices();

        int adapterIndex = m_DeviceParams.AdapterIndex;

        int firstDevice = 0;
        int lastDevice = int(devices.size()) - 1;
        if (adapterIndex >= 0)
        {
            if (adapterIndex > lastDevice)
            {
                CORE_ERROR("The specified Vulkan physical Device {} does not exist.", adapterIndex);
                return false;
            }
            firstDevice = adapterIndex;
            lastDevice = adapterIndex;
        }

        // Start building an error message in case we cannot find a Device.
        std::stringstream errorStream;
        errorStream << "Cannot find a Vulkan Device that supports all the required extensions and properties.";

        // build a list of GPUs
        std::vector<vk::PhysicalDevice> discreteGPUs;
        std::vector<vk::PhysicalDevice> otherGPUs;
        for (int deviceIndex = firstDevice; deviceIndex <= lastDevice; ++deviceIndex)
        {
            vk::PhysicalDevice const& dev = devices[deviceIndex];
            vk::PhysicalDeviceProperties prop = dev.getProperties();

            errorStream << std::endl << prop.deviceName.data() << ":";

            // check that all required Device extensions are present
            std::unordered_set<std::string> requiredExtensions = m_EnabledExtensions.Device;
            auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
            for (const auto& ext : deviceExtensions)
            {
                requiredExtensions.erase(std::string(ext.extensionName.data()));
            }

            bool deviceIsGood = true;

            if (!requiredExtensions.empty())
            {
                // Device is missing one or more required extensions
                for (const auto& ext : requiredExtensions)
                {
                    errorStream << std::endl << "  - missing " << ext;
                }
                deviceIsGood = false;
            }

            auto deviceFeatures = dev.getFeatures();
            if (!deviceFeatures.samplerAnisotropy)
            {
                // Device is a toaster oven
                errorStream << std::endl << "  - does not support samplerAnisotropy";
                deviceIsGood = false;
            }
            if (!deviceFeatures.textureCompressionBC)
            {
                errorStream << std::endl << "  - does not support textureCompressionBC";
                deviceIsGood = false;
            }

            if (!findQueueFamilies(dev))
            {
                // Device doesn't have all the queue families we need
                errorStream << std::endl << "  - does not support the necessary queue types";
                deviceIsGood = false;
            }

            if (deviceIsGood && m_WindowSurface)
            {
                bool surfaceSupported = dev.getSurfaceSupportKHR(m_PresentQueueFamily, m_WindowSurface);
                if (!surfaceSupported)
                {
                    errorStream << std::endl << "  - does not support the window surface";
                    deviceIsGood = false;
                }
                else
                {
                    // check that this Device supports our intended swap chain creation parameters
                    auto surfaceCaps = dev.getSurfaceCapabilitiesKHR(m_WindowSurface);
                    auto surfaceFmts = dev.getSurfaceFormatsKHR(m_WindowSurface);

                    if (surfaceCaps.minImageCount > m_DeviceParams.SwapChainBufferCount ||
                        (surfaceCaps.maxImageCount < m_DeviceParams.SwapChainBufferCount && surfaceCaps.maxImageCount > 0))
                    {
                        errorStream << std::endl << "  - cannot support the requested swap chain image count:";
                        errorStream << " requested " << m_DeviceParams.SwapChainBufferCount << ", available " << surfaceCaps.minImageCount << " - " << surfaceCaps.maxImageCount;
                        deviceIsGood = false;
                    }

                    if (surfaceCaps.minImageExtent.width > requestedExtent.width ||
                        surfaceCaps.minImageExtent.height > requestedExtent.height ||
                        surfaceCaps.maxImageExtent.width < requestedExtent.width ||
                        surfaceCaps.maxImageExtent.height < requestedExtent.height)
                    {
                        errorStream << std::endl << "  - cannot support the requested swap chain size:";
                        errorStream << " requested " << requestedExtent.width << "x" << requestedExtent.height << ", ";
                        errorStream << " available " << surfaceCaps.minImageExtent.width << "x" << surfaceCaps.minImageExtent.height;
                        errorStream << " - " << surfaceCaps.maxImageExtent.width << "x" << surfaceCaps.maxImageExtent.height;
                        deviceIsGood = false;
                    }

                    bool surfaceFormatPresent = false;
                    for (const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts)
                    {
                        if (surfaceFmt.format == vk::Format(requestedFormat))
                        {
                            surfaceFormatPresent = true;
                            break;
                        }
                    }

                    if (!surfaceFormatPresent)
                    {
                        // can't create a swap chain using the format requested
                        errorStream << std::endl << "  - does not support the requested swap chain format";
                        deviceIsGood = false;
                    }

                    // check that we can present from the graphics queue
                    u32 canPresent = dev.getSurfaceSupportKHR(m_GraphicsQueueFamily, m_WindowSurface);
                    if (!canPresent)
                    {
                        errorStream << std::endl << "  - cannot present";
                        deviceIsGood = false;
                    }
                }
            }

            if (!deviceIsGood)
                continue;

            if (prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                discreteGPUs.push_back(dev);
            }
            else
            {
                otherGPUs.push_back(dev);
            }
        }

        // pick the first discrete GPU if it exists, otherwise the first integrated GPU
        if (!discreteGPUs.empty())
        {
            u32 selectedIndex = 0;
            m_VulkanPhysicalDevice = discreteGPUs[selectedIndex];
            return true;
        }

        if (!otherGPUs.empty())
        {
            u32 selectedIndex = 0;

            m_VulkanPhysicalDevice = otherGPUs[selectedIndex];
            return true;
        }

        CORE_ERROR("{}", errorStream.str().c_str());

        return false;
    }

    bool VulkanDeviceManager::findQueueFamilies(vk::PhysicalDevice physicalDevice)
    {
        auto props = physicalDevice.getQueueFamilyProperties();

        for (int i = 0; i < int(props.size()); i++)
        {
            const auto& queueFamily = props[i];

            if (m_GraphicsQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_GraphicsQueueFamily = i;
                }
            }

            if (m_ComputeQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_ComputeQueueFamily = i;
                }
            }

            if (m_TransferQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_TransferQueueFamily = i;
                }
            }

            if (m_PresentQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    glfwGetPhysicalDevicePresentationSupport(m_VulkanInstance, physicalDevice, i))
                {
                    m_PresentQueueFamily = i;
                }
            }
        }

        if (m_GraphicsQueueFamily == -1 ||
            (m_PresentQueueFamily == -1 && !m_DeviceParams.HeadlessDevice) ||
            (m_ComputeQueueFamily == -1 && m_DeviceParams.EnableComputeQueue) ||
            (m_TransferQueueFamily == -1 && m_DeviceParams.EnableCopyQueue))
        {
            return false;
        }

        return true;
    }

    bool VulkanDeviceManager::createDevice()
    {
        // figure out which optional extensions are supported
        auto deviceExtensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
        for (const auto& ext : deviceExtensions)
        {
            const std::string name = ext.extensionName;
            if (m_OptionalExtensions.Device.find(name) != m_OptionalExtensions.Device.end())
            {
                if (name == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME && m_DeviceParams.HeadlessDevice)
                    continue;

                m_EnabledExtensions.Device.insert(name);
            }

            if (m_DeviceParams.EnableRayTracingExtensions && m_RayTracingExtensions.find(name) != m_RayTracingExtensions.end())
            {
                m_EnabledExtensions.Device.insert(name);
            }
        }

        if (!m_DeviceParams.HeadlessDevice)
        {
            m_EnabledExtensions.Device.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        const vk::PhysicalDeviceProperties physicalDeviceProperties = m_VulkanPhysicalDevice.getProperties();
        m_RendererString = std::string(physicalDeviceProperties.deviceName.data());

        bool accelStructSupported = false;
        bool rayPipelineSupported = false;
        bool rayQuerySupported = false;
        bool meshletsSupported = false;
        bool vrsSupported = false;
        bool interlockSupported = false;
        bool barycentricSupported = false;
        bool storage16BitSupported = false;
        bool synchronization2Supported = false;
        bool maintenance4Supported = false;
        bool aftermathSupported = false;

        CORE_INFO("Enabled Vulkan Device extensions:");
        for (const auto& ext : m_EnabledExtensions.Device)
        {
            CORE_INFO("    {}", ext.c_str());

            if (ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                accelStructSupported = true;
            else if (ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                rayPipelineSupported = true;
            else if (ext == VK_KHR_RAY_QUERY_EXTENSION_NAME)
                rayQuerySupported = true;
            else if (ext == VK_NV_MESH_SHADER_EXTENSION_NAME)
                meshletsSupported = true;
            else if (ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)
                vrsSupported = true;
            else if (ext == VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME)
                interlockSupported = true;
            else if (ext == VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME)
                barycentricSupported = true;
            else if (ext == VK_KHR_16BIT_STORAGE_EXTENSION_NAME)
                storage16BitSupported = true;
            else if (ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                synchronization2Supported = true;
            else if (ext == VK_KHR_MAINTENANCE_4_EXTENSION_NAME)
                maintenance4Supported = true;
            else if (ext == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME)
                m_SwapChainMutableFormatSupported = true;
            else if (ext == VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME)
                aftermathSupported = true;
        }

#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
        void* pNext = nullptr;

        vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
        // Determine support for Buffer Device Address, the Vulkan 1.2 way
        auto bufferDeviceAddressFeatures = vk::PhysicalDeviceBufferDeviceAddressFeatures();
        // Determine support for maintenance4
        auto maintenance4Features = vk::PhysicalDeviceMaintenance4Features();
        // Determine support for aftermath
        auto aftermathPhysicalFeatures = vk::PhysicalDeviceDiagnosticsConfigFeaturesNV();

        // Put the user-provided extension structure at the end of the chain
        pNext = m_DeviceParams.PhysicalDeviceFeatures2Extensions;
        APPEND_EXTENSION(true, bufferDeviceAddressFeatures);
        APPEND_EXTENSION(maintenance4Supported, maintenance4Features);
        APPEND_EXTENSION(aftermathSupported, aftermathPhysicalFeatures);

        physicalDeviceFeatures2.pNext = pNext;
        m_VulkanPhysicalDevice.getFeatures2(&physicalDeviceFeatures2);

        std::unordered_set<int> uniqueQueueFamilies = {
            m_GraphicsQueueFamily };

        if (!m_DeviceParams.HeadlessDevice)
            uniqueQueueFamilies.insert(m_PresentQueueFamily);

        if (m_DeviceParams.EnableComputeQueue)
            uniqueQueueFamilies.insert(m_ComputeQueueFamily);

        if (m_DeviceParams.EnableCopyQueue)
            uniqueQueueFamilies.insert(m_TransferQueueFamily);

        float priority = 1.f;
        std::vector<vk::DeviceQueueCreateInfo> queueDesc;
        queueDesc.reserve(uniqueQueueFamilies.size());
        for (int queueFamily : uniqueQueueFamilies)
        {
            queueDesc.push_back(vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(queueFamily)
                .setQueueCount(1)
                .setPQueuePriorities(&priority));
        }

        auto accelStructFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
            .setAccelerationStructure(true);
        auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
            .setRayTracingPipeline(true)
            .setRayTraversalPrimitiveCulling(true);
        auto rayQueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR()
            .setRayQuery(true);
        auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
            .setTaskShader(true)
            .setMeshShader(true);
        auto interlockFeatures = vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT()
            .setFragmentShaderPixelInterlock(true);
        auto barycentricFeatures = vk::PhysicalDeviceFragmentShaderBarycentricFeaturesKHR()
            .setFragmentShaderBarycentric(true);
        auto storage16BitFeatures = vk::PhysicalDevice16BitStorageFeatures()
            .setStorageBuffer16BitAccess(true);
        auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
            .setPipelineFragmentShadingRate(true)
            .setPrimitiveFragmentShadingRate(true)
            .setAttachmentFragmentShadingRate(true);
        auto vulkan13features = vk::PhysicalDeviceVulkan13Features()
            .setSynchronization2(synchronization2Supported)
            .setMaintenance4(maintenance4Features.maintenance4);
        auto aftermathFeatures = vk::DeviceDiagnosticsConfigCreateInfoNV()
            .setFlags(vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking
                | vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo
                | vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderErrorReporting);

        pNext = nullptr;
        APPEND_EXTENSION(accelStructSupported, accelStructFeatures)
            APPEND_EXTENSION(rayPipelineSupported, rayPipelineFeatures)
            APPEND_EXTENSION(rayQuerySupported, rayQueryFeatures)
            APPEND_EXTENSION(meshletsSupported, meshletFeatures)
            APPEND_EXTENSION(vrsSupported, vrsFeatures)
            APPEND_EXTENSION(interlockSupported, interlockFeatures)
            APPEND_EXTENSION(barycentricSupported, barycentricFeatures)
            APPEND_EXTENSION(storage16BitSupported, storage16BitFeatures)
            APPEND_EXTENSION(physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3, vulkan13features)
            APPEND_EXTENSION(physicalDeviceProperties.apiVersion < VK_API_VERSION_1_3 && maintenance4Supported, maintenance4Features);
#undef APPEND_EXTENSION

        auto deviceFeatures = vk::PhysicalDeviceFeatures()
            .setShaderImageGatherExtended(true)
            .setSamplerAnisotropy(true)
            .setTessellationShader(true)
            .setTextureCompressionBC(true)
            .setGeometryShader(true)
            .setImageCubeArray(true)
            .setShaderInt16(true)
            .setFillModeNonSolid(true)
            .setFragmentStoresAndAtomics(true)
            .setDualSrcBlend(true)
            .setVertexPipelineStoresAndAtomics(true);

        // Add a Vulkan 1.1 structure with default settings to make it easier for apps to modify them
        auto vulkan11features = vk::PhysicalDeviceVulkan11Features()
            .setPNext(pNext);

        auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
            .setDescriptorIndexing(true)
            .setRuntimeDescriptorArray(true)
            .setDescriptorBindingPartiallyBound(true)
            .setDescriptorBindingVariableDescriptorCount(true)
            .setTimelineSemaphore(true)
            .setShaderSampledImageArrayNonUniformIndexing(true)
            .setBufferDeviceAddress(bufferDeviceAddressFeatures.bufferDeviceAddress)
            .setPNext(&vulkan11features);

        auto layerVec = StringSetToVector(m_EnabledExtensions.Layers);
        auto extVec = StringSetToVector(m_EnabledExtensions.Device);

        auto deviceDesc = vk::DeviceCreateInfo()
            .setPQueueCreateInfos(queueDesc.data())
            .setQueueCreateInfoCount(u32(queueDesc.size()))
            .setPEnabledFeatures(&deviceFeatures)
            .setEnabledExtensionCount(u32(extVec.size()))
            .setPpEnabledExtensionNames(extVec.data())
            .setEnabledLayerCount(u32(layerVec.size()))
            .setPpEnabledLayerNames(layerVec.data())
            .setPNext(&vulkan12features);

        if (m_DeviceParams.DeviceCreateInfoCallback)
            m_DeviceParams.DeviceCreateInfoCallback(deviceDesc);

        const vk::Result res = m_VulkanPhysicalDevice.createDevice(&deviceDesc, nullptr, &m_VulkanDevice);
        if (res != vk::Result::eSuccess)
        {
            CORE_ERROR("Failed to create a Vulkan physical Device, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
            return false;
        }

        m_VulkanDevice.getQueue(m_GraphicsQueueFamily, kGraphicsQueueIndex, &m_GraphicsQueue);
        if (m_DeviceParams.EnableComputeQueue)
            m_VulkanDevice.getQueue(m_ComputeQueueFamily, kComputeQueueIndex, &m_ComputeQueue);
        if (m_DeviceParams.EnableCopyQueue)
            m_VulkanDevice.getQueue(m_TransferQueueFamily, kTransferQueueIndex, &m_TransferQueue);
        if (!m_DeviceParams.HeadlessDevice)
            m_VulkanDevice.getQueue(m_PresentQueueFamily, kPresentQueueIndex, &m_PresentQueue);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanDevice);

        // remember the bufferDeviceAddress feature enablement
        m_BufferDeviceAddressSupported = vulkan12features.bufferDeviceAddress;

        CORE_INFO("Created Vulkan Device: {}", m_RendererString.c_str());

        return true;
    }

    bool VulkanDeviceManager::createWindowSurface()
    {
        const VkResult res = glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, (VkSurfaceKHR*)&m_WindowSurface);
        if (res != VK_SUCCESS)
        {
            CORE_ERROR("Failed to create a GLFW window surface, error code = {}", nvrhi::vulkan::resultToString(res));
            return false;
        }

        return true;
    }

    void VulkanDeviceManager::destroySwapChain()
    {
        if (m_VulkanDevice)
        {
            m_VulkanDevice.waitIdle();
        }

        if (m_SwapChain)
        {
            m_VulkanDevice.destroySwapchainKHR(m_SwapChain);
            m_SwapChain = nullptr;
        }

        m_SwapChainImages.clear();
    }

    bool VulkanDeviceManager::createSwapChain()
    {
        destroySwapChain();

        m_SwapChainFormat = {
            vk::Format(nvrhi::vulkan::convertFormat(m_DeviceParams.SwapChainFormat)),
            vk::ColorSpaceKHR::eSrgbNonlinear
        };

        vk::Extent2D extent = vk::Extent2D(m_DeviceParams.BackBufferWidth, m_DeviceParams.BackBufferHeight);

        std::unordered_set<u32> uniqueQueues = {
            u32(m_GraphicsQueueFamily),
            u32(m_PresentQueueFamily) };

        std::vector<u32> queues = SetToVector(uniqueQueues);

        const bool enableSwapChainSharing = queues.size() > 1;

        auto desc = vk::SwapchainCreateInfoKHR()
            .setSurface(m_WindowSurface)
            .setMinImageCount(m_DeviceParams.SwapChainBufferCount)
            .setImageFormat(m_SwapChainFormat.format)
            .setImageColorSpace(m_SwapChainFormat.colorSpace)
            .setImageExtent(extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setImageSharingMode(enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
            .setFlags(m_SwapChainMutableFormatSupported ? vk::SwapchainCreateFlagBitsKHR::eMutableFormat : vk::SwapchainCreateFlagBitsKHR(0))
            .setQueueFamilyIndexCount(enableSwapChainSharing ? u32(queues.size()) : 0)
            .setPQueueFamilyIndices(enableSwapChainSharing ? queues.data() : nullptr)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(m_DeviceParams.VsyncEnabled ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate)
            .setClipped(true)
            .setOldSwapchain(nullptr);

        std::vector<vk::Format> imageFormats = { m_SwapChainFormat.format };
        switch (m_SwapChainFormat.format)
        {
        case vk::Format::eR8G8B8A8Unorm:
            imageFormats.push_back(vk::Format::eR8G8B8A8Srgb);
            break;
        case vk::Format::eR8G8B8A8Srgb:
            imageFormats.push_back(vk::Format::eR8G8B8A8Unorm);
            break;
        case vk::Format::eB8G8R8A8Unorm:
            imageFormats.push_back(vk::Format::eB8G8R8A8Srgb);
            break;
        case vk::Format::eB8G8R8A8Srgb:
            imageFormats.push_back(vk::Format::eB8G8R8A8Unorm);
            break;
        default:
            break;
        }

        auto imageFormatListCreateInfo = vk::ImageFormatListCreateInfo()
            .setViewFormats(imageFormats);

        if (m_SwapChainMutableFormatSupported)
            desc.pNext = &imageFormatListCreateInfo;

        const vk::Result res = m_VulkanDevice.createSwapchainKHR(&desc, nullptr, &m_SwapChain);
        if (res != vk::Result::eSuccess)
        {
            CORE_ERROR("Failed to create a Vulkan swap chain, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
            return false;
        }

        // retrieve swap chain images
        auto images = m_VulkanDevice.getSwapchainImagesKHR(m_SwapChain);
        for (auto image : images)
        {
            SwapChainImage sci;
            sci.image = image;

            nvrhi::TextureDesc textureDesc;
            textureDesc.width = m_DeviceParams.BackBufferWidth;
            textureDesc.height = m_DeviceParams.BackBufferHeight;
            textureDesc.format = m_DeviceParams.SwapChainFormat;
            textureDesc.debugName = "Swap chain image";
            textureDesc.initialState = nvrhi::ResourceStates::Present;
            textureDesc.keepInitialState = true;
            textureDesc.isRenderTarget = true;

            sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.image), textureDesc);
            m_SwapChainImages.push_back(sci);
        }

        m_SwapChainIndex = 0;

        return true;
    }

#define CHECK(a) if (!(a)) { return false; }

    bool VulkanDeviceManager::CreateInstanceInternal()
    {

        if (m_DeviceParams.EnableDebugRuntime)
        {
            m_EnabledExtensions.Instance.insert("VK_EXT_debug_report");
            m_EnabledExtensions.Layers.insert("VK_LAYER_KHRONOS_validation");
        }

        m_DynamicLoader = std::make_unique<VulkanDynamicLoader>(m_DeviceParams.VulkanLibraryName);

        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
            m_DynamicLoader->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        return createInstance();
    }

    bool VulkanDeviceManager::EnumerateAdapters(std::vector<AdapterInfo>& outAdapters)
    {
        if (!m_VulkanInstance)
            return false;

        std::vector<vk::PhysicalDevice> devices = m_VulkanInstance.enumeratePhysicalDevices();
        outAdapters.clear();

        for (auto physicalDevice : devices)
        {
            vk::PhysicalDeviceProperties2 properties2;
            vk::PhysicalDeviceIDProperties idProperties;
            properties2.pNext = &idProperties;
            physicalDevice.getProperties2(&properties2);

            auto const& properties = properties2.properties;

            AdapterInfo adapterInfo;
            adapterInfo.Name = properties.deviceName.data();
            adapterInfo.VendorID = properties.vendorID;
            adapterInfo.DeviceID = properties.deviceID;
            adapterInfo.vkPhysicalDevice = physicalDevice;
            adapterInfo.DedicatedVideoMemory = 0;

            AdapterInfo::UUID uuid;
            static_assert(uuid.size() == idProperties.deviceUUID.size());
            memcpy(uuid.data(), idProperties.deviceUUID.data(), uuid.size());
            adapterInfo.Uuid = uuid;

            if (idProperties.deviceLUIDValid)
            {
                AdapterInfo::LUID luid;
                static_assert(luid.size() == idProperties.deviceLUID.size());
                memcpy(luid.data(), idProperties.deviceLUID.data(), luid.size());
                adapterInfo.Luid = luid;
            }

            // Go through the memory types to figure out the amount of VRAM on this physical Device.
            vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
            for (u32 heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex)
            {
                vk::MemoryHeap const& heap = memoryProperties.memoryHeaps[heapIndex];
                if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal)
                {
                    adapterInfo.DedicatedVideoMemory += heap.size;
                }
            }

            outAdapters.push_back(std::move(adapterInfo));
        }

        return true;
    }

    bool VulkanDeviceManager::CreateDevice()
    {
        if (m_DeviceParams.EnableDebugRuntime)
        {
            installDebugCallback();
        }

        // add Device extensions requested by the user
        for (const std::string& name : m_DeviceParams.RequiredVulkanDeviceExtensions)
        {
            m_EnabledExtensions.Device.insert(name);
        }
        for (const std::string& name : m_DeviceParams.OptionalVulkanDeviceExtensions)
        {
            m_OptionalExtensions.Device.insert(name);
        }

        if (!m_DeviceParams.HeadlessDevice)
        {
            // Need to adjust the swap chain format before creating the Device because it affects physical Device selection
            if (m_DeviceParams.SwapChainFormat == nvrhi::Format::SRGBA8_UNORM)
                m_DeviceParams.SwapChainFormat = nvrhi::Format::SBGRA8_UNORM;
            else if (m_DeviceParams.SwapChainFormat == nvrhi::Format::RGBA8_UNORM)
                m_DeviceParams.SwapChainFormat = nvrhi::Format::BGRA8_UNORM;

            CHECK(createWindowSurface())
        }
        CHECK(pickPhysicalDevice())
            CHECK(findQueueFamilies(m_VulkanPhysicalDevice))
            CHECK(createDevice())

            auto vecInstanceExt = StringSetToVector(m_EnabledExtensions.Instance);
        auto vecLayers = StringSetToVector(m_EnabledExtensions.Layers);
        auto vecDeviceExt = StringSetToVector(m_EnabledExtensions.Device);

        nvrhi::vulkan::DeviceDesc deviceDesc;
        deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
        deviceDesc.instance = m_VulkanInstance;
        deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
        deviceDesc.device = m_VulkanDevice;
        deviceDesc.graphicsQueue = m_GraphicsQueue;
        deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
        if (m_DeviceParams.EnableComputeQueue)
        {
            deviceDesc.computeQueue = m_ComputeQueue;
            deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
        }
        if (m_DeviceParams.EnableCopyQueue)
        {
            deviceDesc.transferQueue = m_TransferQueue;
            deviceDesc.transferQueueIndex = m_TransferQueueFamily;
        }
        deviceDesc.instanceExtensions = vecInstanceExt.data();
        deviceDesc.numInstanceExtensions = vecInstanceExt.size();
        deviceDesc.deviceExtensions = vecDeviceExt.data();
        deviceDesc.numDeviceExtensions = vecDeviceExt.size();
        deviceDesc.bufferDeviceAddressSupported = m_BufferDeviceAddressSupported;
        deviceDesc.vulkanLibraryName = m_DeviceParams.VulkanLibraryName;

        m_NvrhiDevice = nvrhi::vulkan::createDevice(deviceDesc);

        if (m_DeviceParams.EnableNvrhiValidationLayer)
        {
            m_ValidationLayer = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
        }


        return true;
    }

    bool VulkanDeviceManager::CreateSwapChain()
    {
        CHECK(createSwapChain())

            m_PresentSemaphores.reserve(m_DeviceParams.MaxFramesInFlight + 1);
        m_AcquireSemaphores.reserve(m_DeviceParams.MaxFramesInFlight + 1);
        for (u32 i = 0; i < m_DeviceParams.MaxFramesInFlight + 1; ++i)
        {
            m_PresentSemaphores.push_back(m_VulkanDevice.createSemaphore(vk::SemaphoreCreateInfo()));
            m_AcquireSemaphores.push_back(m_VulkanDevice.createSemaphore(vk::SemaphoreCreateInfo()));
        }

        return true;
    }
#undef CHECK

    void VulkanDeviceManager::DestroyDeviceAndSwapChain()
    {
        destroySwapChain();

        for (auto& semaphore : m_PresentSemaphores)
        {
            if (semaphore)
            {
                m_VulkanDevice.destroySemaphore(semaphore);
                semaphore = vk::Semaphore();
            }
        }

        for (auto& semaphore : m_AcquireSemaphores)
        {
            if (semaphore)
            {
                m_VulkanDevice.destroySemaphore(semaphore);
                semaphore = vk::Semaphore();
            }
        }

        m_NvrhiDevice = nullptr;
        m_ValidationLayer = nullptr;
        m_RendererString.clear();

        if (m_VulkanDevice)
        {
            m_VulkanDevice.destroy();
            m_VulkanDevice = nullptr;
        }

        if (m_WindowSurface)
        {
            assert(m_VulkanInstance);
            m_VulkanInstance.destroySurfaceKHR(m_WindowSurface);
            m_WindowSurface = nullptr;
        }

        if (m_DebugReportCallback)
        {
            m_VulkanInstance.destroyDebugReportCallbackEXT(m_DebugReportCallback);
        }

        if (m_VulkanInstance)
        {
            m_VulkanInstance.destroy();
            m_VulkanInstance = nullptr;
        }
    }

    bool VulkanDeviceManager::BeginFrame()
    {
        const auto& semaphore = m_AcquireSemaphores[m_AcquireSemaphoreIndex];

        vk::Result res;

        int const maxAttempts = 3;
        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            res = m_VulkanDevice.acquireNextImageKHR(
                m_SwapChain,
                std::numeric_limits<uint64_t>::max(), // timeout
                semaphore,
                vk::Fence(),
                &m_SwapChainIndex);

            if ((res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) && attempt < maxAttempts)
            {
                BackBufferResizing();
                auto surfaceCaps = m_VulkanPhysicalDevice.getSurfaceCapabilitiesKHR(m_WindowSurface);

                m_DeviceParams.BackBufferWidth = surfaceCaps.currentExtent.width;
                m_DeviceParams.BackBufferHeight = surfaceCaps.currentExtent.height;

                ResizeSwapChain();
                BackBufferResized();
            }
            else
                break;
        }

        m_AcquireSemaphoreIndex = (m_AcquireSemaphoreIndex + 1) % m_AcquireSemaphores.size();

        if (res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR) // Suboptimal is considered a success
        {
            // Schedule the wait. The actual wait operation will be submitted when the app executes any command list.
            m_NvrhiDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);
            return true;
        }

        return false;
    }

    bool VulkanDeviceManager::Present()
    {
        const auto& semaphore = m_PresentSemaphores[m_PresentSemaphoreIndex];

        m_NvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);

        // NVRHI buffers the semaphores and signals them when something is submitted to a queue.
        // Call 'executeCommandLists' with no command lists to actually signal the semaphore.
        m_NvrhiDevice->executeCommandLists(nullptr, 0);

        vk::PresentInfoKHR info = vk::PresentInfoKHR()
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&semaphore)
            .setSwapchainCount(1)
            .setPSwapchains(&m_SwapChain)
            .setPImageIndices(&m_SwapChainIndex);

        const vk::Result res = m_PresentQueue.presentKHR(&info);
        if (!(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR))
        {
            return false;
        }

        m_PresentSemaphoreIndex = (m_PresentSemaphoreIndex + 1) % m_PresentSemaphores.size();

#ifndef PLATFORM_WINDOWS
        if (m_DeviceParams.VsyncEnabled || m_DeviceParams.EnableDebugRuntime)
        {
            // according to vulkan-tutorial.com, "the validation layer implementation expects
            // the application to explicitly synchronize with the GPU"
            m_PresentQueue.waitIdle();
        }
#endif

        while (m_FramesInFlight.size() >= m_DeviceParams.MaxFramesInFlight)
        {
            auto query = m_FramesInFlight.front();
            m_FramesInFlight.pop();

            m_NvrhiDevice->waitEventQuery(query);

            m_QueryPool.push_back(query);
        }

        nvrhi::EventQueryHandle query;
        if (!m_QueryPool.empty())
        {
            query = m_QueryPool.back();
            m_QueryPool.pop_back();
        }
        else
        {
            query = m_NvrhiDevice->createEventQuery();
        }

        m_NvrhiDevice->resetEventQuery(query);
        m_NvrhiDevice->setEventQuery(query, nvrhi::CommandQueue::Graphics);
        m_FramesInFlight.push(query);
        return true;
    }

    DeviceManager* DeviceManager::CreateVK()
    {
        return new VulkanDeviceManager();
    }

}