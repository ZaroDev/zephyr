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

        return true;
    }
    void Renderer::Shutdown()
    {
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

        m_CommandList->Open();
        nvrhi::utils::ClearColorAttachment(m_CommandList->GetNvrhiCommandList(), framebuffer, 0, nvrhi::Color(0.1f, 0.1f, 0.15f, 1.0f));
        m_CommandList->Close();

        m_DeviceHandle->GetNvrhiDevice()->executeCommandList(m_CommandList->GetNvrhiCommandList());

        m_Surface->Present();
    }
    void Renderer::PostRenderer(float deltaTime)
    {
    }
}