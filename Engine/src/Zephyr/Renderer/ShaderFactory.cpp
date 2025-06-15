#include "pch.h"
#include "ShaderFactory.h"



namespace Zephyr
{
    ShaderFactory::ShaderFactory(nvrhi::DeviceHandle device, Ref<IFileSystem> fs, const Path& path)
        : m_Device(device), m_FileSystem(fs), m_BasePath(path)
    {
        const SlangResult success = slang::createGlobalSession(m_SlangGlobalSession.writeRef());
        if (SLANG_FAILED(success))
        {
            CORE_ERROR("Failed to create global session for shader factory");
        }
    }

    ShaderFactory::~ShaderFactory() {}
    void ShaderFactory::ClearCache()
    {
        m_ByteCodeCache.clear();
    }
    Ref<IBlob> ShaderFactory::GetByteCode(const char* fileName, const char* entryName)
    {
        if (!entryName)
        {
            entryName = "main";
        }

        String ajustedName = fileName;
        Path shaderFilePath = m_BasePath / (ajustedName + ".bin");
        Ref<IBlob>& data = m_ByteCodeCache[shaderFilePath.generic_string()];
        
        if (data)
        {
            return data;
        }

        data = m_FileSystem->ReadFile(shaderFilePath);

        if (!data)
        {
            CORE_ERROR("Couldn't read the binary file for the shader {0} from {1}", fileName, shaderFilePath.generic_string().c_str());
            return nullptr;
        }

        return data;
    }
    bool ShaderFactory::CompileShader(const char* fileName, const char* entryName)
    {
        using Slang::ComPtr;
        if (!IsShaderExtension(fileName))
        {
            CORE_ERROR("Trying to compile shader with invalid shader extension: {}!", fileName);
            return false;
        }
        const Path shaderFilePath = m_BasePath / String(fileName);
        if (!m_FileSystem->FileExists(shaderFilePath))
        {
            CORE_ERROR("Trying to compile a shader with an invalid path {}", shaderFilePath);
            return false;
        }
        slang::SessionDesc desc{};
        slang::TargetDesc targetDesc{};
        targetDesc.flags = 0;
        switch (m_Device->getGraphicsAPI())
        {
        case nvrhi::GraphicsAPI::D3D11:
            targetDesc.format = SLANG_DXBC;
            targetDesc.profile = m_SlangGlobalSession->findProfile("sm_5_1");
	        break;
        case nvrhi::GraphicsAPI::D3D12:
            targetDesc.format = SLANG_DXIL;
            targetDesc.profile = m_SlangGlobalSession->findProfile("sm_6_7");
	        break;
        case nvrhi::GraphicsAPI::VULKAN:
            targetDesc.format = SLANG_SPIRV;
            targetDesc.profile = m_SlangGlobalSession->findProfile("spirv_1_5");
	        break;
        }

        desc.targets = &targetDesc;
        desc.targetCount = 1;
        desc.compilerOptionEntryCount = 0;
        ComPtr<slang::ISession> session;
        const SlangResult sessionSuccess = m_SlangGlobalSession->createSession(desc, session.writeRef());
        if (SLANG_FAILED(sessionSuccess))
        {
            CORE_ERROR("Failed to create slang session");
            return false;
        }

        slang::IModule* slangModule = nullptr;
        {
            ComPtr<slang::IBlob> diagnosticBlob;
            slangModule = session->loadModule(shaderFilePath.generic_string().c_str(), diagnosticBlob.writeRef());
            if (!slangModule)
            {
                return false;
            }
        }

        ComPtr<slang::IEntryPoint> entryPoint;
        slangModule->findEntryPointByName(entryName, entryPoint.writeRef());

        std::vector<slang::IComponentType*> componentTypes;
        componentTypes.emplace_back(slangModule);
        componentTypes.emplace_back(entryPoint);

        ComPtr<slang::IComponentType> composedProgram;
        {
            ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = session->createCompositeComponentType(
                componentTypes.data(),
                componentTypes.size(),
                composedProgram.writeRef(),
                diagnosticsBlob.writeRef());
            if (diagnosticsBlob)
            {
                CORE_ERROR("Error while trying to create composite component type {}", static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
            }

            if (SLANG_FAILED(result))
            {
                CORE_ERROR("Failed to created composed program");
                return false;
            }
        }

        ComPtr<slang::IBlob> byteCode;
        {
            ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = composedProgram->getEntryPointCode(
                0,
                0,
                byteCode.writeRef(),
                diagnosticsBlob.writeRef());
            if (diagnosticsBlob)
            {
                CORE_ERROR("Error while trying to get entry point code {}", static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
            }

            if (SLANG_FAILED(result))
            {
                CORE_ERROR("Failed to get byte code");
                return false;
            }
        }

        const Path outputPath = m_BasePath / (String(fileName) + ".bin");
        m_FileSystem->WriteFile(outputPath, byteCode.readRef(), byteCode->getBufferSize());
        Ref<IBlob>& data = m_ByteCodeCache[shaderFilePath.generic_string()];
        data = m_FileSystem->ReadFile(outputPath);
        if (!data)
        {
            CORE_ERROR("Failed to write shader output file");
            return false;
        }


        return true;
    }


    nvrhi::ShaderHandle ShaderFactory::CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc)
    {
        Ref<IBlob> byteCode = GetByteCode(fileName, entryName);
        if (!byteCode)
        {
			if (CompileShader(fileName, entryName))
			{
                byteCode = GetByteCode(fileName, entryName);
			}
            else
            {
				return nullptr;
            }
        }

        nvrhi::ShaderDesc descCopy = desc;
        descCopy.entryName = entryName;
        if (descCopy.debugName.empty())
        {
            descCopy.debugName = entryName;
        }


        return CreateStaticShader(StaticShader{ byteCode->Data(), byteCode->Size() }, pDefines, descCopy);
    }
    nvrhi::ShaderHandle ShaderFactory::CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType)
    {
        return CreateShader(fileName, entryName, pDefines, nvrhi::ShaderDesc().setShaderType(shaderType));
    }
    nvrhi::ShaderLibraryHandle ShaderFactory::CreateShaderLibrary(const char* fileName, const std::vector<ShaderMacro>* pDefines)
    {
        std::shared_ptr<IBlob> byteCode = GetByteCode(fileName, nullptr);

        if (!byteCode)
            return nullptr;

        return CreateStaticShaderLibrary(StaticShader{ byteCode->Data(), byteCode->Size() }, pDefines);
    }

    nvrhi::ShaderHandle ShaderFactory::CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro>* pDefines,
	    const nvrhi::ShaderDesc& desc)
    {
        if (!shader.ByteCode || !shader.Size)
            return nullptr;

        return m_Device->createShader(desc, shader.ByteCode, shader.Size);
    }

    nvrhi::ShaderHandle ShaderFactory::CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro>* pDefines,
	    nvrhi::ShaderType shaderType)
    {
        return CreateStaticShader(shader, pDefines, nvrhi::ShaderDesc().setShaderType(shaderType));
    }

    nvrhi::ShaderHandle ShaderFactory::CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil,
	    StaticShader spirv, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc)
    {
        StaticShader shader;
        switch (m_Device->getGraphicsAPI())
        {
        case nvrhi::GraphicsAPI::D3D11:
            shader = dxbc;
            break;
        case nvrhi::GraphicsAPI::D3D12:
            shader = dxil;
            break;
        case nvrhi::GraphicsAPI::VULKAN:
            shader = spirv;
            break;
        }

        return CreateStaticShader(shader, pDefines, desc);
    }

    nvrhi::ShaderHandle ShaderFactory::CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil,
	    StaticShader spirv, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType)
    {
        return CreateStaticPlatformShader(dxbc, dxil, spirv, pDefines, nvrhi::ShaderDesc().setShaderType(shaderType));
    }

    nvrhi::ShaderLibraryHandle ShaderFactory::CreateStaticShaderLibrary(StaticShader shader,
	    const std::vector<ShaderMacro>* pDefines)
    {

        if (!shader.ByteCode || !shader.Size)
            return nullptr;

        return m_Device->createShaderLibrary(shader.ByteCode, shader.Size);
    }

    nvrhi::ShaderLibraryHandle ShaderFactory::CreateStaticPlatformShaderLibrary(StaticShader dxil, StaticShader spirv,
	    const std::vector<ShaderMacro>* pDefines)
    {
        StaticShader shader;
        switch (m_Device->getGraphicsAPI())
        {
        case nvrhi::GraphicsAPI::D3D12:
            shader = dxil;
            break;
        case nvrhi::GraphicsAPI::VULKAN:
            shader = spirv;
            break;
        default:
            break;
        }

        return CreateStaticShaderLibrary(shader, pDefines);
    }

    nvrhi::ShaderHandle ShaderFactory::CreateAutoShader(const char* fileName, const char* entryName, StaticShader dxbc,
	    StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc)
    {
        nvrhi::ShaderDesc descCopy = desc;
        descCopy.entryName = entryName;
        if (descCopy.debugName.empty())
            descCopy.debugName = fileName;

        nvrhi::ShaderHandle shader = CreateStaticPlatformShader(dxbc, dxil, spirv, pDefines, descCopy);
        if (shader)
            return shader;

        return CreateShader(fileName, entryName, pDefines, desc);
    }

    nvrhi::ShaderHandle ShaderFactory::CreateAutoShader(const char* fileName, const char* entryName, StaticShader dxbc,
	    StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType)
    {

        return CreateAutoShader(fileName, entryName, dxbc, dxil, spirv, pDefines, nvrhi::ShaderDesc().setShaderType(shaderType));
    }

    nvrhi::ShaderLibraryHandle ShaderFactory::CreateAutoShaderLibrary(const char* fileName, StaticShader dxil,
	    StaticShader spirv, const std::vector<ShaderMacro>* pDefines)
    {
        nvrhi::ShaderLibraryHandle shader = CreateStaticPlatformShaderLibrary(dxil, spirv, pDefines);
        if (shader)
            return shader;

        return CreateShaderLibrary(fileName, pDefines);
    }

    std::pair<const void*, size_t> ShaderFactory::FindShaderFromHash(uint64_t hash, std::function<uint64_t(std::pair<const void*, size_t>, nvrhi::GraphicsAPI)> hashGenerator)
    {
        return std::make_pair(nullptr, 0);
    }

    bool ShaderFactory::IsShaderExtension(const char* fileName) const
    {
        const Path& filePath = fileName;

        if (filePath.extension() == ".slang")
        {
            return true;
        }

        return false;
    }
}
