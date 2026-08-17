#include "pch.h"
#include "Renderer.h"

#include <nvrhi/utils.h>
#include <GLFW/glfw3.h>

namespace Zephyr
{
    Renderer::Renderer(const RendererData& params)
        : m_Data(params)
    {
    }
    bool Renderer::Initialize()
    {
        m_DeviceHandle = Device::Create(m_Data.Api);
        if (!m_DeviceHandle)
        {
            CORE_ERROR("Failed to create a {} graphics device", GetGraphicsName(m_Data.Api));
            return false;
        }

        m_Surface = m_DeviceHandle->CreateSurface(m_Data.Window);
        if (!m_Surface)
        {
            CORE_ERROR("Failed to create a rendering surface");
            return false;
        }

        m_CommandList = CreateScope<CommandList>(m_DeviceHandle->GetNvrhiDevice());

        m_ShaderFactory = CreateScope<ShaderFactory>(m_DeviceHandle->GetNvrhiDevice(), CreateRef<DefaultFileSystem>(), Path("Shaders"));

        m_VertexShader = m_ShaderFactory->CreateShader("Triangle.vs.slang", "main", nullptr, nvrhi::ShaderType::Vertex);
        m_PixelShader = m_ShaderFactory->CreateShader("Triangle.ps.slang", "main", nullptr, nvrhi::ShaderType::Pixel);
        if (!m_VertexShader || !m_PixelShader)
        {
            CORE_ERROR("Failed to compile the triangle shaders");
            return false;
        }

        return true;
    }
    void Renderer::Shutdown()
    {
        m_Pipeline = nullptr;
        m_PixelShader = nullptr;
        m_VertexShader = nullptr;
        m_ShaderFactory.reset();
        m_CommandList.reset();
        m_Surface.reset();
        m_DeviceHandle.reset();
    }
    void Renderer::PreUpdate(float deltaTime)
    {
    }
    void Renderer::Update(float deltaTime)
    {
    }
    void Renderer::PostUpdate(float deltaTime)
    {
    }
    void Renderer::PreRender(float deltaTime)
    {
    }
    void Renderer::Render(float deltaTime)
    {
        int width, height;
        glfwGetFramebufferSize(m_Data.Window->GetGLFWHandle(), &width, &height);
        if (width <= 0 || height <= 0)
            return;

        if (u32(width) != m_Surface->GetWidth() || u32(height) != m_Surface->GetHeight())
        {
            m_Surface->Resize(u32(width), u32(height));
        }

        if (!m_Surface->AcquireNextImage())
            return;

        nvrhi::IFramebuffer* framebuffer = m_Surface->GetCurrentFramebuffer();
        if (!framebuffer)
            return;

        if (!m_Pipeline)
        {
            nvrhi::RenderState renderState;
            renderState.rasterState.setCullNone();
            renderState.depthStencilState.disableDepthTest().disableDepthWrite();

            auto psoDesc = nvrhi::GraphicsPipelineDesc()
                .setVertexShader(m_VertexShader)
                .setPixelShader(m_PixelShader)
                .setRenderState(renderState)
                .setPrimType(nvrhi::PrimitiveType::TriangleList);

            m_Pipeline = m_DeviceHandle->GetNvrhiDevice()->createGraphicsPipeline(psoDesc, framebuffer);
            if (!m_Pipeline)
            {
                CORE_ERROR("Failed to create the triangle graphics pipeline");
                return;
            }
        }

        m_CommandList->Open();
        nvrhi::utils::ClearColorAttachment(m_CommandList->GetNvrhiCommandList(), framebuffer, 0, nvrhi::Color(0.1f, 0.1f, 0.15f, 1.0f));

        nvrhi::GraphicsState state;
        state.pipeline = m_Pipeline;
        state.framebuffer = framebuffer;
        state.viewport.addViewportAndScissorRect(nvrhi::Viewport(float(m_Surface->GetWidth()), float(m_Surface->GetHeight())));
        m_CommandList->GetNvrhiCommandList()->setGraphicsState(state);

        nvrhi::DrawArguments drawArgs;
        drawArgs.vertexCount = 3;
        m_CommandList->GetNvrhiCommandList()->draw(drawArgs);

        m_CommandList->Close();

        m_DeviceHandle->GetNvrhiDevice()->executeCommandList(m_CommandList->GetNvrhiCommandList());

        m_Surface->Present();
    }
    void Renderer::PostRenderer(float deltaTime)
    {
    }
}