#pragma once
#include "DeviceManager.h"

namespace Zephyr
{
    struct ShaderMacro
    {
        String Name;
        String Definition;

        ShaderMacro(const String& name, const String& definition)
            : Name(name), Definition(definition){}
    };
    
    class ShaderFactory
    {
    public:
        ShaderFactory(nvrhi::DeviceHandle device, const Path& path);
        virtual ~ShaderFactory();

        void ClearCache();
        Ref<Buffer> GetByteCode(const char* fileName, const char* entryName);

        // Creates a shader from binary file.
        nvrhi::ShaderHandle CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // A version of CreateShader that takes a ShaderType instead of a full ShaderDesc.
        nvrhi::ShaderHandle CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType);

        // Creates a shader library from binary file.
        nvrhi::ShaderLibraryHandle CreateShaderLibrary(const char* fileName, const std::vector<ShaderMacro>* pDefines);

        // Looks up a shader binary based on a provided hash and the function used to generate it
        std::pair<const void*, size_t> FindShaderFromHash(uint64_t hash, std::function<uint64_t(std::pair<const void*, size_t>, nvrhi::GraphicsAPI)> hashGenerator);
        
    private:
        nvrhi::DeviceHandle m_Device;
        std::unordered_map<String, Ref<Buffer>> m_ByteCodeCache;
        Path m_BasePath;
    };
}