#pragma once

#include <imgui.h>

#include <Zephyr/RHI/Device.h>
#include <Zephyr/RHI/CommandBuffer.h>
#include <Zephyr/Renderer/ShaderFactory.h>

namespace Zephyr
{
	class RegisteredFont
	{
	protected:
		friend class ImGuiRenderer;

		Ref<IBlob> m_Data;
		bool const m_IsDefault;
		bool const m_IsCompressed;
		float const m_SizeAtDefaultScale;
		ImFont* m_ImFont = nullptr;

		void CreateScaledFont(float displayScale);
		void ReleaseScaledFont();
	public:
		// Creates an invalid font that will not add any ImGui fonts
		RegisteredFont()
			: m_IsDefault(false), m_IsCompressed(false), m_SizeAtDefaultScale(0.f)
		{
		}

		// Creates a default font with the given size
		RegisteredFont(float size)
			: m_IsDefault(true), m_IsCompressed(false), m_SizeAtDefaultScale(size)
		{
		}

		// Creates a custom font
		RegisteredFont(Ref<IBlob> data, bool isCompressed, float size)
			: m_Data(data), m_IsDefault(false), m_IsCompressed(isCompressed), m_SizeAtDefaultScale(size)
		{
		}

		bool HasFontData() const { return m_Data != nullptr; }

		// Returned pointer is transient (changes when DPI changes or new fonts load) - do not
		// cache it between frames.
		ImFont* GetScaledFont() const { return m_ImFont; }
	};

	// Renders Dear ImGui draw data via RHI::Device/RHI::CommandList. Replaces the deleted
	// DeviceManager/IRenderPass-based ImGuiRenderPass (see tasks/plan.md Task 16) - the font
	// atlas upload, buffer growth, binding-set cache, and PSO setup logic below is recovered
	// from git history (pre-Task-15 ImGuiRenderPass.cpp/h), adapted off DeviceManager/IRenderPass
	// onto RHI::Device and a dedicated RHI::CommandList. Owns the ImGui context itself, same as
	// the original ImGuiRenderPass constructor did.
	class ImGuiRenderer
	{
	public:
		ImGuiRenderer(const Ref<Device>& device, ShaderFactory& shaderFactory);
		~ImGuiRenderer();

		Ref<RegisteredFont> CreateFontFromFile(IFileSystem& fs, const Path& fontFile, float fontSize);
		Ref<RegisteredFont> CreateFontFromMemory(void const* pData, size_t size, float fontSize);
		Ref<RegisteredFont> CreateFontFromMemoryCompressed(void const* pData, size_t size, float fontSize);
		Ref<RegisteredFont> GetDefaultFont() const { return m_DefaultFont; }

		// windowWidth/Height and scaleX/Y come from the owning Renderer's Window/Surface - this
		// class has no window/DPI knowledge of its own. Calls ImGui::NewFrame(); the caller
		// builds UI between this and Render().
		void Animate(float elapsedTimeSeconds, u32 windowWidth, u32 windowHeight, float scaleX, float scaleY);

		// Calls ImGui::Render() and rasterizes the resulting draw data into framebuffer via an
		// internally-owned RHI::CommandList.
		void Render(nvrhi::IFramebuffer* framebuffer);

		void BackBufferResizing();

		// Original had a SupportExplicitDisplayScaling on/off toggle sourced from
		// DeviceCreationParameters, which RHI has no equivalent of - dropped, DPI rescaling is
		// now always active (see Animate()).
		void DisplayScaleChanged(float scaleX, float scaleY);

	private:
		Ref<RegisteredFont> CreateFontFromMemoryInternal(void const* pData, size_t size, bool compressed, float fontSize);

		bool UpdateFontTexture();
		bool ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer);
		nvrhi::IGraphicsPipeline* GetPSO(nvrhi::IFramebuffer* framebuffer);
		nvrhi::IBindingSet* GetBindingSet(nvrhi::ITexture* texture);
		bool UpdateGeometry(nvrhi::ICommandList* commandList);

		Ref<Device> m_Device;
		Scope<CommandList> m_CommandList;

		nvrhi::ShaderHandle m_VertexShader;
		nvrhi::ShaderHandle m_PixelShader;
		nvrhi::InputLayoutHandle m_ShaderAttribLayout;

		nvrhi::TextureHandle m_FontTexture;
		nvrhi::SamplerHandle m_FontSampler;

		nvrhi::BufferHandle m_VertexBuffer;
		nvrhi::BufferHandle m_IndexBuffer;

		nvrhi::BindingLayoutHandle m_BindingLayout;
		nvrhi::GraphicsPipelineDesc m_BasePSODesc;
		nvrhi::GraphicsPipelineHandle m_Pso;
		std::unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> m_BindingsCache;

		std::vector<ImDrawVert> m_VtxScratch;
		std::vector<ImDrawIdx> m_IdxScratch;

		std::vector<Ref<RegisteredFont>> m_Fonts;
		Ref<RegisteredFont> m_DefaultFont;
	};
}
