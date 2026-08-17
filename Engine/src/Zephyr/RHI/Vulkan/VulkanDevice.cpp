#include "pch.h"
#include "VulkanDevice.h"

#include <sstream>

#include <GLFW/glfw3.h>
#include <nvrhi/validation.h>

// NOTE: VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE (the actual global dispatcher
// definition, required exactly once per binary) currently lives in VulkanDeviceManager.cpp.
// Do not add it here while that file still exists - it would be a duplicate symbol. When
// VulkanDeviceManager.cpp is deleted (tasks/plan.md Task 15), move the storage macro here.

namespace Zephyr
{
	static std::vector<const char*> StringSetToVector(const std::unordered_set<std::string>& set)
	{
		std::vector<const char*> ret;
		for (const auto& s : set)
		{
			ret.push_back(s.c_str());
		}

		return ret;
	}

	VulkanDevice::VulkanDevice()
	{
		if (!CreateVulkanInstance())
		{
			CORE_CRITICAL("Failed to create a Vulkan instance");
			return;
		}

#ifdef DEBUG
		InstallDebugCallback();
#endif

		if (!PickPhysicalDevice())
		{
			CORE_CRITICAL("Failed to find a suitable Vulkan physical device");
			return;
		}

		// PickPhysicalDevice() evaluates queue families per-candidate while filtering; re-run it
		// on the device actually selected so m_GraphicsQueueFamily/m_PresentQueueFamily reflect it.
		if (!FindQueueFamilies(m_VulkanPhysicalDevice))
		{
			CORE_CRITICAL("Selected Vulkan physical device is missing a required queue family");
			return;
		}

		if (!CreateLogicalDevice())
		{
			CORE_CRITICAL("Failed to create a Vulkan logical device");
			return;
		}
	}

	VulkanDevice::~VulkanDevice()
	{
		m_NvrhiDevice = nullptr;
		m_ValidationLayer = nullptr;

		if (m_VulkanDevice)
		{
			m_VulkanDevice.destroy();
			m_VulkanDevice = nullptr;
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

	Ref<Surface> VulkanDevice::CreateSurface(const Ref<Window>& window)
	{
		// See tasks/plan.md Task 5 - RHI::VulkanSurface is not implemented yet.
		return Ref<Surface>();
	}

	bool VulkanDevice::CreateVulkanInstance()
	{
		m_DynamicLoader = std::make_unique<VulkanDynamicLoader>();

		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
			m_DynamicLoader->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

#ifdef DEBUG
		m_EnabledExtensions.Instance.insert("VK_EXT_debug_report");
		m_EnabledExtensions.Layers.insert("VK_LAYER_KHRONOS_validation");
#endif

		if (!glfwVulkanSupported())
		{
			CORE_ERROR("GLFW reports that Vulkan is not supported. Perhaps missing a call to glfwInit()?");
			return false;
		}

		// add any extensions required by GLFW
		u32 glfwExtCount;
		const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		CORE_ASSERT(glfwExt, "glfwGetRequiredInstanceExtensions failed");

		for (u32 i = 0; i < glfwExtCount; i++)
		{
			m_EnabledExtensions.Instance.insert(std::string(glfwExt[i]));
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

		// Query the Vulkan API version supported on the system to make sure we use at least 1.3.
		vk::Result res = vk::enumerateInstanceVersion(&applicationInfo.apiVersion);
		if (res != vk::Result::eSuccess)
		{
			CORE_ERROR("Call to vkEnumerateInstanceVersion failed, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
			return false;
		}

		const u32 minimumVulkanVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
		if (applicationInfo.apiVersion < minimumVulkanVersion)
		{
			CORE_ERROR("The Vulkan API version supported on the system ({0}.{1}.{2}) is too low, at least {3}.{4}.{5} is required.",
				VK_API_VERSION_MAJOR(applicationInfo.apiVersion), VK_API_VERSION_MINOR(applicationInfo.apiVersion), VK_API_VERSION_PATCH(applicationInfo.apiVersion),
				VK_API_VERSION_MAJOR(minimumVulkanVersion), VK_API_VERSION_MINOR(minimumVulkanVersion), VK_API_VERSION_PATCH(minimumVulkanVersion));
			return false;
		}

		if (VK_API_VERSION_VARIANT(applicationInfo.apiVersion) != 0)
		{
			CORE_ERROR("The Vulkan API supported on the system uses an unexpected variant: {}", VK_API_VERSION_VARIANT(applicationInfo.apiVersion));
			return false;
		}

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

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		vk::DebugReportFlagsEXT flags,
		vk::DebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData)
	{
		CORE_WARN("[Vulkan: location=0x{0} code={1}, layerPrefix='{2}'] {3}", location, code, layerPrefix, msg);
		return VK_FALSE;
	}

	void VulkanDevice::InstallDebugCallback()
	{
		auto info = vk::DebugReportCallbackCreateInfoEXT()
			.setFlags(vk::DebugReportFlagBitsEXT::eError |
				vk::DebugReportFlagBitsEXT::eWarning |
				vk::DebugReportFlagBitsEXT::ePerformanceWarning)
			.setPfnCallback(VulkanDebugCallback)
			.setPUserData(this);

		vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT(&info, nullptr, &m_DebugReportCallback);
		CORE_ASSERT(res == vk::Result::eSuccess, "Failed to install the Vulkan debug report callback");
	}

	bool VulkanDevice::PickPhysicalDevice()
	{
		auto devices = m_VulkanInstance.enumeratePhysicalDevices();

		// Start building an error message in case we cannot find a Device.
		std::stringstream errorStream;
		errorStream << "Cannot find a Vulkan Device that supports all the required extensions and properties.";

		std::vector<vk::PhysicalDevice> discreteGPUs;
		std::vector<vk::PhysicalDevice> otherGPUs;
		for (const vk::PhysicalDevice& dev : devices)
		{
			vk::PhysicalDeviceProperties prop = dev.getProperties();

			errorStream << std::endl << prop.deviceName.data() << ":";

			std::unordered_set<std::string> requiredExtensions = m_EnabledExtensions.Device;
			auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
			for (const auto& ext : deviceExtensions)
			{
				requiredExtensions.erase(std::string(ext.extensionName.data()));
			}

			bool deviceIsGood = true;

			if (!requiredExtensions.empty())
			{
				for (const auto& ext : requiredExtensions)
				{
					errorStream << std::endl << "  - missing " << ext;
				}
				deviceIsGood = false;
			}

			auto deviceFeatures = dev.getFeatures();
			if (!deviceFeatures.samplerAnisotropy)
			{
				errorStream << std::endl << "  - does not support samplerAnisotropy";
				deviceIsGood = false;
			}
			if (!deviceFeatures.textureCompressionBC)
			{
				errorStream << std::endl << "  - does not support textureCompressionBC";
				deviceIsGood = false;
			}

			if (!FindQueueFamilies(dev))
			{
				errorStream << std::endl << "  - does not support the necessary queue types";
				deviceIsGood = false;
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
			m_VulkanPhysicalDevice = discreteGPUs[0];
			return true;
		}

		if (!otherGPUs.empty())
		{
			m_VulkanPhysicalDevice = otherGPUs[0];
			return true;
		}

		CORE_ERROR("{}", errorStream.str().c_str());
		return false;
	}

	bool VulkanDevice::FindQueueFamilies(vk::PhysicalDevice physicalDevice)
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

			if (m_PresentQueueFamily == -1)
			{
				if (queueFamily.queueCount > 0 &&
					glfwGetPhysicalDevicePresentationSupport(m_VulkanInstance, physicalDevice, i))
				{
					m_PresentQueueFamily = i;
				}
			}
		}

		return m_GraphicsQueueFamily != -1 && m_PresentQueueFamily != -1;
	}

	bool VulkanDevice::CreateLogicalDevice()
	{
		// figure out which optional extensions are supported
		auto deviceExtensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
		for (const auto& ext : deviceExtensions)
		{
			const std::string name = ext.extensionName;
			if (m_OptionalExtensions.Device.find(name) != m_OptionalExtensions.Device.end())
			{
				m_EnabledExtensions.Device.insert(name);
			}
		}

		m_EnabledExtensions.Device.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		const vk::PhysicalDeviceProperties physicalDeviceProperties = m_VulkanPhysicalDevice.getProperties();
		m_RendererString = std::string(physicalDeviceProperties.deviceName.data());

		bool maintenance4Supported = false;
		bool synchronization2Supported = false;

		CORE_INFO("Enabled Vulkan Device extensions:");
		for (const auto& ext : m_EnabledExtensions.Device)
		{
			CORE_INFO("    {}", ext.c_str());

			if (ext == VK_KHR_MAINTENANCE_4_EXTENSION_NAME)
				maintenance4Supported = true;
			else if (ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
				synchronization2Supported = true;
			else if (ext == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME)
				m_SwapChainMutableFormatSupported = true;
		}

		vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
		auto bufferDeviceAddressFeatures = vk::PhysicalDeviceBufferDeviceAddressFeatures();
		auto maintenance4Features = vk::PhysicalDeviceMaintenance4Features();

		void* pNext = nullptr;
#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
		APPEND_EXTENSION(true, bufferDeviceAddressFeatures);
		APPEND_EXTENSION(maintenance4Supported, maintenance4Features);

		physicalDeviceFeatures2.pNext = pNext;
		m_VulkanPhysicalDevice.getFeatures2(&physicalDeviceFeatures2);

		std::unordered_set<int> uniqueQueueFamilies = { m_GraphicsQueueFamily, m_PresentQueueFamily };

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

		// The instance is already required to be Vulkan 1.3+ (see CreateVulkanInstance), so the
		// physical device's own apiVersion is expected to be >= 1.3 too - request synchronization2
		// and maintenance4 via the 1.3 feature struct rather than maintenance4Features standalone.
		auto vulkan13features = vk::PhysicalDeviceVulkan13Features()
			.setSynchronization2(synchronization2Supported)
			.setMaintenance4(maintenance4Supported);

		pNext = nullptr;
		APPEND_EXTENSION(physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3, vulkan13features);
		APPEND_EXTENSION(physicalDeviceProperties.apiVersion < VK_API_VERSION_1_3 && maintenance4Supported, maintenance4Features);
#undef APPEND_EXTENSION

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

		auto vkDeviceDesc = vk::DeviceCreateInfo()
			.setPQueueCreateInfos(queueDesc.data())
			.setQueueCreateInfoCount(u32(queueDesc.size()))
			.setPEnabledFeatures(&deviceFeatures)
			.setEnabledExtensionCount(u32(extVec.size()))
			.setPpEnabledExtensionNames(extVec.data())
			.setEnabledLayerCount(u32(layerVec.size()))
			.setPpEnabledLayerNames(layerVec.data())
			.setPNext(&vulkan12features);

		const vk::Result res = m_VulkanPhysicalDevice.createDevice(&vkDeviceDesc, nullptr, &m_VulkanDevice);
		if (res != vk::Result::eSuccess)
		{
			CORE_ERROR("Failed to create a Vulkan logical Device, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
			return false;
		}

		m_VulkanDevice.getQueue(m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
		m_VulkanDevice.getQueue(m_PresentQueueFamily, 0, &m_PresentQueue);

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanDevice);

		m_BufferDeviceAddressSupported = vulkan12features.bufferDeviceAddress;

		CORE_INFO("Created Vulkan Device: {}", m_RendererString.c_str());

		auto vecInstanceExt = StringSetToVector(m_EnabledExtensions.Instance);
		auto vecDeviceExt = StringSetToVector(m_EnabledExtensions.Device);

		nvrhi::vulkan::DeviceDesc nvrhiDeviceDesc;
		nvrhiDeviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
		nvrhiDeviceDesc.instance = m_VulkanInstance;
		nvrhiDeviceDesc.physicalDevice = m_VulkanPhysicalDevice;
		nvrhiDeviceDesc.device = m_VulkanDevice;
		nvrhiDeviceDesc.graphicsQueue = m_GraphicsQueue;
		nvrhiDeviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
		nvrhiDeviceDesc.instanceExtensions = vecInstanceExt.data();
		nvrhiDeviceDesc.numInstanceExtensions = vecInstanceExt.size();
		nvrhiDeviceDesc.deviceExtensions = vecDeviceExt.data();
		nvrhiDeviceDesc.numDeviceExtensions = vecDeviceExt.size();
		nvrhiDeviceDesc.bufferDeviceAddressSupported = m_BufferDeviceAddressSupported;

		m_NvrhiDevice = nvrhi::vulkan::createDevice(nvrhiDeviceDesc);

#ifdef DEBUG
		m_ValidationLayer = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
#endif

		return true;
	}
}
