#pragma once
#include <Zephyr/Core/Base.h>
#include <Zephyr/Core/Assert.h>


namespace Zephyr
{
	struct RenderDevice
	{
		enum class Kind : u8
		{
			OTHER = 0,
			INTEGRATED = 1,
			DISCRETE = 2,
			VIRTUAL = 3,
			CPU = 4,
		};

		String Name = "Unknown";
		u32 VendorId = 0;
		u32 UsedVRAM = 0;
		u32 AvailableVRAM = 0;
		u32 TotalVRAM = 0;
		Kind PhysicalKind = Kind::OTHER;

		bool IsNvidia() const
		{
			CORE_ASSERT(VendorId != 0);
			return VendorId == 0x10DE;
		}
		bool IsAMD() const
		{
			CORE_ASSERT(VendorId != 0);
			return VendorId == 0x1002;
		}
		bool IsIntel() const
		{
			CORE_ASSERT(VendorId != 0);
			return VendorId == 0x8086;
		}

		StrView VendorIdName() const 
		{
			switch (VendorId)
			{
			case 0x1002: return "AMD";
			case 0x1010: return "ImgTec";
			case 0x10DE: return "NVIDIA";
			case 0x13B5: return "ARM";
			case 0x5143: return "Qualcomm";
			case 0x8086: return "Intel";
			default: return "Unknown";
			}
		}

	};
}