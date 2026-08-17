#pragma once

#include <Zephyr/Window/Window.h>
#include <Zephyr/RHI/GraphicsAPI.h>
#include <Zephyr/RHI/Surface.h>

#include <nvrhi/nvrhi.h>

namespace Zephyr
{
	struct DefaultMessageCallback : public nvrhi::IMessageCallback
	{
		static DefaultMessageCallback& GetInstance();
		virtual void message(nvrhi::MessageSeverity severity, const char* messageText) override;
	};

	class Device
	{
	public:
		static Ref<Device> Create(GraphicsAPI api);

		virtual ~Device() = default;

		virtual GraphicsAPI GetGraphicsAPI() const = 0;
		virtual Ref<Surface> CreateSurface(const Ref<Window>& window) = 0;

		// The underlying nvrhi device. Anything above nvrhi (ShaderFactory, PSO/resource
		// creation, ...) goes through this handle rather than a backend-specific type.
		virtual nvrhi::DeviceHandle GetNvrhiDevice() const = 0;
	};
}