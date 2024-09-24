#pragma once
#ifdef PLATFORM_WINDOWS
namespace Zephyr
{
	namespace FileDialogs::Windows
	{
		Zephyr::Path OpenFile(Zephyr::StrView filter);
		Zephyr::Path SaveFile(Zephyr::StrView filter);
	}
}

#endif