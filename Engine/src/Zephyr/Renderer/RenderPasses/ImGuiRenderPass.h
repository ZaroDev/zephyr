#pragma once
#include <Zephyr/Renderer/IRenderPass.h>
#include <imgui.h>
namespace Zephyr
{
    // From donut
    struct ImGui_NVRHI
    {
        nvrhi::DeviceHandle m_device;
        nvrhi::CommandListHandle m_commandList;

        nvrhi::ShaderHandle vertexShader;
        nvrhi::ShaderHandle pixelShader;
        nvrhi::InputLayoutHandle shaderAttribLayout;

        nvrhi::TextureHandle fontTexture;
        nvrhi::SamplerHandle fontSampler;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;

        nvrhi::BindingLayoutHandle bindingLayout;
        nvrhi::GraphicsPipelineDesc basePSODesc;

        nvrhi::GraphicsPipelineHandle pso;
        std::unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> bindingsCache;

        std::vector<ImDrawVert> vtxBuffer;
        std::vector<ImDrawIdx> idxBuffer;

        bool Init(nvrhi::IDevice* device, std::shared_ptr<engine::ShaderFactory> shaderFactory);
        bool UpdateFontTexture();
        bool Render(nvrhi::IFramebuffer* framebuffer);
        void BackbufferResizing();

    private:
        bool ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer);


        nvrhi::IGraphicsPipeline* GetPSO(nvrhi::IFramebuffer* fb);
        nvrhi::IBindingSet* GetBindingSet(nvrhi::ITexture* texture);
        bool UpdateGeometry(nvrhi::ICommandList* commandList);
    };

	class ImGuiRenderPass : public IRenderPass
	{
		
	};
}
