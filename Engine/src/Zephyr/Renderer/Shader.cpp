#include <pch.h>
#include "Shader.h"
#include "Renderer.h"
#include <Zephyr/Renderer/Platform/D3D11/D3D11Shader.h>
#include <Zephyr/Renderer/Platform/OpenGL/OpenGLShader.h>

#include <Zephyr/Core/Application.h>


namespace Zephyr
{
    constexpr StrView c_EngineShaders[] =
    {
        "FullScreenQuad",
        "Geometry",
    };

    static_assert(_countof(c_EngineShaders) == EngineShader::MAX);


    Ref<Shader> Shader::Create(const std::filesystem::path& path)
    {
        switch (Renderer::GetAPI())
        {
        case Zephyr::GraphicsAPI::OPENGL: return CreateRef<OpenGL::OpenGLShader>(path);
        case Zephyr::GraphicsAPI::DX11: return CreateRef<D3D11::D3D11Shader>(path);
        }

        CORE_ASSERT(false, "Unkown GraphicsAPI!");
        return nullptr;
    }

    bool ShaderLibrary::LoadEngineShaders()
    {
        for (size i = 0; i < EngineShader::MAX; i++) 
        {
            Load(c_EngineShaders[i]);
        }

        return m_Library.size() == EngineShader::MAX;
    }
    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
    {
        CORE_ASSERT(!Exists(name), "Shader already exists");
        m_Library[name] = shader;
    }
    void ShaderLibrary::Add(const Ref<Shader>& shader)
    {
        auto& name = shader->Name();
        Add(name, shader);
    }
    Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& fileName)
    {
        auto shader = Shader::Create(fileName);
        Add(shader);
        return shader;
    }
    Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::filesystem::path& fileName)
    {
        auto shader = Shader::Create(fileName);
        Add(name, shader);
        return shader;
    }
    Ref<Shader> ShaderLibrary::Get(const std::string& name)
    {
        CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Library[name];
    }
    bool ShaderLibrary::Exists(const std::string& name)
    {
        return m_Library.contains(name);
    }
    
}