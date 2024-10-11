#include <pch.h>
#include "DataConversion.h"

u32 Zephyr::BytesToMB(u64 bytes)
{
	return bytes / 1024 / 1024;
}
