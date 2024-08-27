#pragma once
#include <Zephyr/Asset/Asset.h>
#include <Zephyr/FileSystem/Buffer.h>


namespace Zephyr
{
	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F
	};

	struct TextureSpecification
	{
		u32 Width = 1;
		u32 Height = 1;
		u32 Channels = 4;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
	};

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual u32 GetWidth() const = 0;
		virtual u32 GetHeight() const = 0;
		virtual u32 GetRendererID() const = 0;

		virtual void SetData(void* data, u32 size) = 0;

		virtual void Bind(u32 slot = 0) const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureSpecification& specification, Buffer data = Buffer());

		static AssetType GetStaticType() { return AssetType::TEXTURE2D; }
		virtual AssetType GetType() const override { return GetStaticType(); }
	};
}