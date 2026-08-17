#include <pch.h>
#include "Device.h"

#include <Zephyr/RHI/Vulkan/VulkanDevice.h>

namespace Zephyr
{
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

	Ref<Device> Device::Create(GraphicsAPI api)
	{
		switch (api)
		{
		case Zephyr::GraphicsAPI::D3D11:
			// No RHI::D3D11Device implementation yet - see tasks/plan.md Phase 3.
			return nullptr;
		case Zephyr::GraphicsAPI::D3D12:
			// No RHI::D3D12Device implementation yet - see tasks/plan.md Phase 4.
			return nullptr;
		case Zephyr::GraphicsAPI::VULKAN: return CreateRef<VulkanDevice>();
		}

		CORE_ASSERT(false, "Unknown GraphicsAPI");
		return nullptr;
	}
}