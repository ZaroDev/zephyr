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
#include <Zephyr.h>


namespace Zephyr
{
	class BlobStreamReader
	{
	public:
		DISABLE_MOVE_AND_COPY(BlobStreamReader);

		explicit BlobStreamReader(const u8* buffer)
			: m_Buffer(buffer), m_Position(buffer)
		{
			CORE_ASSERT(buffer);
		}

		template<typename T>
		NODISCARD T Read()
		{
			static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type.");
			T value = *(T*)m_Position;
			m_Position += sizeof(T);
			return value;
		}

		void Read(u8* buffer, Size length)
		{
			memcpy(buffer, m_Position, length);
			m_Position += length;
		}

		void Skip(Size offset)
		{
			m_Position += offset;
		}

		NODISCARD constexpr const u8* const BufferStart() const { return m_Buffer; }
		NODISCARD constexpr const u8* const Position() const { return m_Position; }
		NODISCARD constexpr const Size Offset() const { return m_Position - m_Buffer; }

	private:
		const u8* m_Buffer;
		const u8* m_Position;
	};

	class BlobStreamWriter
	{
	public:
		DISABLE_MOVE_AND_COPY(BlobStreamWriter);

		explicit BlobStreamWriter(u8* buffer, Size bufferSize)
			: m_Buffer(buffer), m_Position(buffer), m_BufferSize(bufferSize)
		{
			CORE_ASSERT(buffer && bufferSize);

		}

		template<typename T>
		void Write(T value)
		{
			static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type.");
			CORE_ASSERT(&m_Position[sizeof(T)] < &m_Buffer[m_BufferSize]);
			*((T*)m_Position) = value;
			m_Position += sizeof(T);
		}

		void Write(const char* buffer, Size length)
		{
			CORE_ASSERT(&m_Position[length] <= &m_Buffer[m_BufferSize]);
			memcpy(m_Position, buffer, length);
			m_Position += length;
		}

		void Write(const u8* buffer, Size length)
		{
			CORE_ASSERT(&m_Position[length] <= &m_Buffer[m_BufferSize]);
			memcpy(m_Position, buffer, length);
			m_Position += length;
		}

		void Skip(Size offset)
		{
			m_Position += offset;
		}

		NODISCARD constexpr const u8* const BufferStart() const { return m_Buffer; }
		NODISCARD constexpr const u8* const BufferEnd() const { return &m_Buffer[m_BufferSize]; }
		NODISCARD constexpr const u8* const Position() const { return m_Position; }
		NODISCARD constexpr const Size Offset() const { return m_Position - m_Buffer; }

	private:
		u8* const m_Buffer;
		u8* m_Position;
		Size m_BufferSize;
	};
}