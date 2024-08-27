#pragma once
#include <Zephyr/Asset/Asset.h>

namespace Zephyr
{
	struct AssetMetadata
	{
		AssetType Type = AssetType::NONE;
		Path FilePath;

		operator bool() const { return Type != AssetType::NONE; }
	};
}