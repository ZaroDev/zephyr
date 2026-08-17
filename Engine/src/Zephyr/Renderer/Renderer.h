#pragma once
#include <functional>

#include <Zephyr/Modules/IModule.h>
#include <Zephyr/RHI/Device.h>
#include <Zephyr/RHI/Surface.h>
#include <Zephyr/RHI/CommandBuffer.h>
#include <Zephyr/Renderer/ShaderFactory.h>
#include <Zephyr/Renderer/ImGuiRenderer.h>
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
		ShaderFactory* GetShaderFactory() const { return m_ShaderFactory.get(); }

		// Invoked each frame between ImGuiRenderer::Animate() (ImGui::NewFrame()) and
		// ImGuiRenderer::Render() (ImGui::Render()) - i.e. exactly where UI-building ImGui::Begin()/
		// End() calls are valid. Application wires this to OnImGui() at construction; Application::
		// Render() cannot call OnImGui() itself, since by the time m_ModuleManager.Render() (which
		// runs this Renderer's Render()) returns, ImGui::Render() has already been called for the
		// frame and further ImGui::Begin() calls would hit ImGui's "Forgot to call NewFrame()?" assert.
		void SetImGuiCallback(std::function<void(float)> callback) { m_ImGuiCallback = std::move(callback); }

	private:
		RendererData m_Data = {};

		Ref<Device> m_DeviceHandle;
		Ref<Surface> m_Surface;
		Scope<CommandList> m_CommandList;
		Scope<ShaderFactory> m_ShaderFactory;
		Scope<ImGuiRenderer> m_ImGuiRenderer;
		std::function<void(float)> m_ImGuiCallback;

		nvrhi::ShaderHandle m_VertexShader;
		nvrhi::ShaderHandle m_PixelShader;
		nvrhi::GraphicsPipelineHandle m_Pipeline;
	};
}