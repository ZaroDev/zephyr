/*
MIT License

Copyright (c) 2025 ZaroDev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <Zephyr/Core/BasicTypes.h>

namespace Zephyr
{
	static u64 s_AllocationCount = 0;
	static u64 s_AllocationsSize = 0;
}

void* operator new(std::size_t count)
{
	if (count == 0)
	{
		return nullptr;
	}
	Zephyr::s_AllocationCount++;
	Zephyr::s_AllocationsSize += count;

	return std::malloc(count);
}
void* operator new[](std::size_t count)
{
	if (count == 0)
	{
		return nullptr;
	}
	Zephyr::s_AllocationCount++;
	Zephyr::s_AllocationsSize += count;

	return std::malloc(count);
}


void operator delete(void* pointer) noexcept
{
	if (pointer)
	{
		Zephyr::s_AllocationCount--;
		Zephyr::s_AllocationsSize -= sizeof(pointer);
		std::free(pointer);
	}
}
void operator delete[](void* pointer) noexcept
{
	if (pointer)
	{
		Zephyr::s_AllocationCount--;
		Zephyr::s_AllocationsSize -= sizeof(pointer);
		std::free(pointer);
	}
}