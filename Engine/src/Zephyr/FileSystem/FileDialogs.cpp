#include <pch.h>
#include "FileDialogs.h"

#include "Platform/Windows/WindowsPlatformUtils.h"

namespace Zephyr::FileDialogs
{
	Zephyr::Path OpenFile(Zephyr::StrView filter)
	{
#ifdef PLATFORM_WINDOWS 
		return Windows::OpenFile(filter);
#else
#error "Not implemented for the current platform"
#endif
	}
	Zephyr::Path SaveFile(Zephyr::StrView filter)
	{
#ifdef PLATFORM_WINDOWS 
		return Windows::SaveFile(filter);
#else
#error "Not implemented for the current platform"
#endif
	}
}