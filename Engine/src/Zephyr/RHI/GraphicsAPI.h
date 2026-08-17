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
#include <Zephyr/Core/Base.h>
#include <Zephyr/Core/BasicTypes.h>

#include <nvrhi/nvrhi.h>

namespace Zephyr
{
	enum class GraphicsAPI : u8
	{
		D3D11,
		D3D12,
		VULKAN
	};
	ENUM_CLASS_FLAG_OPERATORS(GraphicsAPI);

	StrView GetGraphicsName(GraphicsAPI api);

	// Converts the engine's GraphicsAPI selector to nvrhi's own enum, needed anywhere
	// engine code has to hand an API selection to nvrhi (e.g. ShaderFactory's target selection).
	nvrhi::GraphicsAPI ToNVRHI(GraphicsAPI api);
}
