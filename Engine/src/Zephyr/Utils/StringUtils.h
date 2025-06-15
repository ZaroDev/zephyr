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
#include <Zephyr/Math/MathTypes.h>


namespace Zephyr
{
	namespace StringUtils {

		// FNV-1a 32bit hashing algorithm.
		constexpr u32 fnv1a_32(char const* s, SizeT count)
		{
			return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
		}

		constexpr SizeT const_strlen(const char* s)
		{
			SizeT size = 0;
			while (s[size]) { size++; };
			return size;
		}

		inline bool StartsWith(StrView const& value, StrView const& beginning)
		{
			if (beginning.size() > value.size())
				return false;

			return std::equal(beginning.begin(), beginning.end(), value.begin());
		}

		inline bool EndsWith(StrView const& value, StrView const& ending)
		{
			if (ending.size() > value.size())
				return false;

			return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
		}

		struct StringHash
		{
			u32 computedHash;

			constexpr StringHash(u32 hash) noexcept : computedHash(hash) {}

			constexpr StringHash(const char* s) noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s, const_strlen(s));
			}
			constexpr StringHash(const char* s, SizeT count)noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s, count);
			}
			constexpr StringHash(StrView s)noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s.data(), s.size());
			}
			StringHash(const StringHash& other) = default;

			constexpr operator u32()noexcept { return computedHash; }
		};

	}
}