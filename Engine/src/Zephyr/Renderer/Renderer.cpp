#include "pch.h"
#include "Renderer.h"

namespace Zephyr
{
    Renderer::Renderer(const RendererData& params)
        : m_Data(params)
    {
    }
    bool Renderer::Initialize()
    {
        m_DeviceHandle = Device::Create(m_Data.Api);

        return true;
    }
    void Renderer::Shutdown()
    {
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
    }
    void Renderer::PostRenderer(float deltaTime)
    {
    }
}