#pragma once
#include <Zephyr/Modules/IModule.h>
#include <Zephyr/RHI/Device.h>
#include <Zephyr/RHI/Surface.h>
#include <Zephyr/RHI/CommandBuffer.h>
#include <Zephyr/Window/Window.h>

namespace Zephyr
{
	struct RendererData
	{
		GraphicsAPI Api = GraphicsAPI::VULKAN;
		Ref<Window> Window = nullptr;
	};

	class Renderer final : public IModule
	{
	public:
		Renderer(const RendererData& params);
		~Renderer() = default;

		DEFAULT_MOVE_AND_COPY(Renderer);

		virtual bool Initialize() override;
		virtual void Shutdown() override;

		virtual String GetName() const override { return "Renderer"; }
		virtual i32 GetPriority() const override { return 1; }
		virtual UpdateFlags GetUpdateFlags() const override { return UpdateFlags::Update | UpdateFlags::RendererUpdate; }
		virtual bool IsCoreModule() const override { return true; }

		virtual void PreUpdate(float deltaTime) override;
		virtual void Update(float deltaTime) override;
		virtual void PostUpdate(float deltaTime) override;

		virtual void PreRender(float deltaTime) override;
		virtual void Render(float deltaTime) override;
		virtual void PostRenderer(float deltaTime) override;

		Ref<Device> GetDevice() const { return m_DeviceHandle; }
		Ref<Surface> GetSurface() const { return m_Surface; }

	private:
		RendererData m_Data = {};

		Ref<Device> m_DeviceHandle;
		Ref<Surface> m_Surface;
		Scope<CommandList> m_CommandList;
	};
}