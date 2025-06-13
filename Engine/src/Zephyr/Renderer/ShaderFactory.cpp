#include "pch.h"
#include "ShaderFactory.h"

namespace Zephyr
{

    ShaderFactory::ShaderFactory(nvrhi::DeviceHandle device, const Path& path)
        : m_Device(device), m_BasePath(path) {}
    ShaderFactory::~ShaderFactory() {}
    void ShaderFactory::ClearCache()
    {
        m_ByteCodeCache.clear();
    }
    Ref<Buffer> ShaderFactory::GetByteCode(const char* fileName, const char* entryName)
    {
        if (!entryName)
        {
            entryName = "main";
        }

        String ajustedName = fileName;
        {
            const size pos = ajustedName.find(".hlsl");
            if (pos != String::npos)
            {
                ajustedName.erase(pos, 5);
            }
            if (entryName && strcmp(entryName, "main"))
            {
                ajustedName += "_" + String(entryName);
            }

        }
        Path shaderFilePath = m_BasePath / (ajustedName + ".bin");
        Ref<Buffer>& data = m_ByteCodeCache[shaderFilePath.generic_string()];
        
        if (data)
        {
            return data;
        }

        data = FileSystem::ReadFile(shaderFilePath);

        if (data)
        {
            CORE_ERROR("Couldn't read the binary file for the shader {0} from {1}", fileName, shaderFilePath.generic_string().c_str());
            return nullptr;
        }

        return data;
    }
    nvrhi::ShaderHandle ShaderFactory::CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc)
    {
        Ref<Buffer> byteCode = ShaderFactory::GetByteCode(fileName, entryName);
        if (!byteCode)
            
    }
    nvrhi::ShaderHandle ShaderFactory::CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType)
    {
        
    }
    nvrhi::ShaderLibraryHandle ShaderFactory::CreateShaderLibrary(const char* fileName, const std::vector<ShaderMacro>* pDefines)
    {
        
    }
    std::pair<const void*, size_t> ShaderFactory::FindShaderFromHash(uint64_t hash, std::function<uint64_t(std::pair<const void*, size_t>, nvrhi::GraphicsAPI)> hashGenerator)
    {
        
    }
}