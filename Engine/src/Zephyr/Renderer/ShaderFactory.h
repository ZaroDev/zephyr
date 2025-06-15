/*
MIT License

Copyright (c) 2025 ZaroDev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include "DeviceManager.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

namespace Zephyr
{
    struct ShaderMacro
    {
        String Name;
        String Definition;

        ShaderMacro(const String& name, const String& definition)
            : Name(name), Definition(definition){}
    };

    struct StaticShader
    {
        void const* ByteCode = nullptr;
        SizeT Size = 0;
    };
    
    class ShaderFactory
    {
    public:
        ShaderFactory(nvrhi::DeviceHandle device, Ref<IFileSystem> fs, const Path& path);
        virtual ~ShaderFactory();

        void ClearCache();
        Ref<IBlob> GetByteCode(const char* fileName, const char* entryName);

        // Compile shader
        bool CompileShader(const char* fileName, const char* entryName);

        // Creates a shader from binary file.
        nvrhi::ShaderHandle CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // A version of CreateShader that takes a ShaderType instead of a full ShaderDesc.
        nvrhi::ShaderHandle CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType);

        // Creates a shader library from binary file.
        nvrhi::ShaderLibraryHandle CreateShaderLibrary(const char* fileName, const std::vector<ShaderMacro>* pDefines);

        // Creates a shader from the bytecode array.
        nvrhi::ShaderHandle CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // A version of CreateStaticShader that takes a ShaderType instead of a full ShaderDesc.
        nvrhi::ShaderHandle CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType);

        // Creates a shader from one of the platform-speficic bytecode arrays, selecting it based on the device's graphics API.
        nvrhi::ShaderHandle CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // A version of CreateStaticPlatformShader that takes a ShaderType instead of a full ShaderDesc.
        nvrhi::ShaderHandle CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType);

        // Creates a shader library from the bytecode array.
        nvrhi::ShaderLibraryHandle CreateStaticShaderLibrary(StaticShader shader, const std::vector<ShaderMacro>* pDefines);

        // Creates a shader library from one of the platform-speficic bytecode arrays, selecting it based on the device's graphics API.
        nvrhi::ShaderLibraryHandle CreateStaticPlatformShaderLibrary(StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines);

        // Tries to create a shader from one of the platform-specific bytecode arrays (calling CreateStaticPlatformShader).
        // If that fails (e.g. there is no static bytecode), creates a shader from the filesystem binary file (calling CreateShader).
        nvrhi::ShaderHandle CreateAutoShader(const char* fileName, const char* entryName, StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // A versoin of CreateAutoShader that takes a ShaderType instead of a full ShaderDesc.
        nvrhi::ShaderHandle CreateAutoShader(const char* fileName, const char* entryName, StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, nvrhi::ShaderType shaderType);

        // Tries to create a shader library from one of the platform-specific bytecode arrays (calling CreateStaticPlatformShaderLibrary).
        // If that fails (e.g. there is no static bytecode), creates a shader library from the filesystem binary file (calling CreateShaderLibrary).
        nvrhi::ShaderLibraryHandle CreateAutoShaderLibrary(const char* fileName, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines);

        // Looks up a shader binary based on a provided hash and the function used to generate it
        std::pair<const void*, size_t> FindShaderFromHash(uint64_t hash, std::function<uint64_t(std::pair<const void*, size_t>, nvrhi::GraphicsAPI)> hashGenerator);
        
    private:
        bool IsShaderExtension(const char* fileName) const;
    private:
        nvrhi::DeviceHandle m_Device;
        Ref<IFileSystem> m_FileSystem;
        std::unordered_map<String, Ref<IBlob>> m_ByteCodeCache;
        Path m_BasePath;
        Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;
    };
}