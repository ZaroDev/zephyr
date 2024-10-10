#pragma once
#include <Zephyr/Core/UUID.h>

namespace Zephyr
{
	using AssetHandle = UUID;
	

	enum class AssetType
	{
		NONE = 0,
		SCENE,
		TEXTURE2D,
		MODEL
	};

	std::string_view AssetTypeToString(AssetType type);
	AssetType AssetTypeFromString(std::string_view assetType);

	template<typename OStream>
	inline OStream& operator<<(OStream& os, Zephyr::AssetType type)
	{
		return os << Zephyr::AssetTypeToString(type);
	}

	class Asset
	{
	public:
		AssetHandle Handle; // Generate handle


		virtual AssetType GetType() const = 0;
	};
}