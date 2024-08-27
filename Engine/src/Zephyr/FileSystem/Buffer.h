#pragma once

#include <Zephyr/Core/BasicTypes.h>
#include <stdlib.h>
#include <cstring>


namespace Zephyr 
{
	struct Buffer
	{
		u8* Data = nullptr;
		u64 Size = 0;

		Buffer() = default;

		Buffer(u64 size)
		{
			Allocate(size);
		}

		Buffer(const void* data, u64 size)
			: Data((u8*)data), Size(size)
		{

		}

		Buffer(const Buffer&) = default;

		static Buffer Copy(Buffer other)
		{
			Buffer result(other.Size);
			memcpy(result.Data, other.Data, other.Size);
			return result;
		}

		void Allocate(u64 size)
		{
			Release();

			Data = (u8*)malloc(size);
			Size = size;
		}

		void Release()
		{
			free(Data);
			Data = nullptr;
			Size = 0;
		}

		template<typename T>
		T* As()
		{
			return (T*)Data;
		}

		operator bool() const
		{
			return (bool)Data;
		}
	};

}