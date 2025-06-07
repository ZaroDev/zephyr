#include <pch.h>
#include "DataConversion.h"

namespace Zephyr
{
	u32 BytesToMB(u64 bytes)
	{
		return static_cast<u32>(bytes) / 1024u / 1024;
	}
}

