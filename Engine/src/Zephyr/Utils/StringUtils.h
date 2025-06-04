#pragma once
#include <Zephyr/Core/Base.h>
#include <Zephyr/Math/MathTypes.h>


namespace Zephyr
{
	namespace StringUtils {

		// FNV-1a 32bit hashing algorithm.
		constexpr u32 fnv1a_32(char const* s, size count)
		{
			return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
		}

		constexpr size const_strlen(const char* s)
		{
			size size = 0;
			while (s[size]) { size++; };
			return size;
		}

		struct StringHash
		{
			u32 computedHash;

			constexpr StringHash(u32 hash) noexcept : computedHash(hash) {}

			constexpr StringHash(const char* s) noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s, const_strlen(s));
			}
			constexpr StringHash(const char* s, size count)noexcept : computedHash(0)
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