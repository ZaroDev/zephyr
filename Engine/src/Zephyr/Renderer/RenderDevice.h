#pragma once
#include <Zephyr/Core/Base.h>


namespace Zephyr
{
	struct RenderDevice
	{
		String Name;
		String Vendor;
		u32 UsedVRAM;
		u32 AvailableVRAM;
		u32 TotalVRAM;
	};
}