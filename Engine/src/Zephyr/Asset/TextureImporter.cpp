#include <pch.h>
#include "TextureImporter.h"

#include <Zephyr/Project/Project.h>
#include <Zephyr/FileSystem/Buffer.h>

#include <Zephyr/Core/Application.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Zephyr
{
    Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
    {
        return LoadTexture2D(Project::GetWorkingDirectory() / metadata.FilePath);
    }
    Ref<Texture2D> TextureImporter::LoadTexture2D(const Path& path)
    {
        i32 width, height, channels;

        if(Application::Get().Specification().WindowData.API != GraphicsAPI::OPENGL)
        {
            stbi_set_flip_vertically_on_load(0);
        }
        else
        {
            stbi_set_flip_vertically_on_load(1);
        }
        
        Buffer data;
        {
            data.Data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
            channels = 4;
        }

        if (data.Data == nullptr) 
        {
            CORE_ERROR("TextureImporter::LoadTexture2D - Could not load texture from filepath {}", path.string());
            return nullptr;
        }

        data.Size = width * height * channels;

        TextureSpecification spec;
        spec.Width = width;
        spec.Height = height;
        spec.Channels = channels;
        
        switch (channels)
        {
        case 3:
            spec.Format = ImageFormat::RGB8;
            break;
        case 4:
            spec.Format = ImageFormat::RGBA8;
            break;
        }

        Ref<Texture2D> texture = Texture2D::Create(spec, data);
        data.Release();

        return texture;
    }
}