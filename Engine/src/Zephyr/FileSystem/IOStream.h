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

		void Read(u8* buffer, size length)
		{
			memcpy(buffer, m_Position, length);
			m_Position += length;
		}

		void Skip(size offset)
		{
			m_Position += offset;
		}

		NODISCARD constexpr const u8* const BufferStart() const { return m_Buffer; }
		NODISCARD constexpr const u8* const Position() const { return m_Position; }
		NODISCARD constexpr const size Offset() const { return m_Position - m_Buffer; }

	private:
		const u8* m_Buffer;
		const u8* m_Position;
	};

	class BlobStreamWriter
	{
	public:
		DISABLE_MOVE_AND_COPY(BlobStreamWriter);

		explicit BlobStreamWriter(u8* buffer, size bufferSize)
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

		void Write(const char* buffer, size length)
		{
			CORE_ASSERT(&m_Position[length] <= &m_Buffer[m_BufferSize]);
			memcpy(m_Position, buffer, length);
			m_Position += length;
		}

		void Write(const u8* buffer, size length)
		{
			CORE_ASSERT(&m_Position[length] <= &m_Buffer[m_BufferSize]);
			memcpy(m_Position, buffer, length);
			m_Position += length;
		}

		void Skip(size offset)
		{
			m_Position += offset;
		}

		NODISCARD constexpr const u8* const BufferStart() const { return m_Buffer; }
		NODISCARD constexpr const u8* const BufferEnd() const { return &m_Buffer[m_BufferSize]; }
		NODISCARD constexpr const u8* const Position() const { return m_Position; }
		NODISCARD constexpr const size Offset() const { return m_Position - m_Buffer; }

	private:
		u8* const m_Buffer;
		u8* m_Position;
		size m_BufferSize;
	};
}