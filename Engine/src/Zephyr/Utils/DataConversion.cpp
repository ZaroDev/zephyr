#include <pch.h>
#include "DataConversion.h"

u32 Zephyr::BytesToMB(u64 bytes)
{
	return static_cast<u32>(bytes) / 1024u / 1024;
}
