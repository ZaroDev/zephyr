#pragma once
#include <Zephyr.h>


namespace Zephyr
{
	class BlobStreamReader
	{
	public:
		DISABLE_COPY(BlobStreamReader);
		DISABLE_MOVE(BlobStreamReader);

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

		void Read(u8* buffer, size_t length)
		{
			memcpy(buffer, m_Position, length);
			m_Position += length;
		}

		void Skip(size_t offset)
		{
			m_Position += offset;
		}

		NODISCARD constexpr const u8* const BufferStart() const { return m_Buffer; }
		NODISCARD constexpr const u8* const Position() const { return m_Position; }
		NODISCARD constexpr const size_t Offset() const { return m_Position - m_Buffer; }

	private:
		const u8* m_Buffer;
		const u8* m_Position;
	};

	class BlobStreamWriter
	{
	public:
		DISABLE_COPY(BlobStreamWriter);
		DISABLE_MOVE(BlobStreamWriter);

		explicit BlobStreamWriter(u8* buffer, size_t bufferSize)
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

		void Write(const char* buffer, size_t length)
		{
			CORE_ASSERT(&m_Position[length] <= &m_Buffer[m_BufferSize]);
			memcpy(m_Position, buffer, length);
			m_Position += length;
		}

		void Write(const u8* buffer, size_t length)
		{
			CORE_ASSERT(&m_Position[length] <= &m_Buffer[m_BufferSize]);
			memcpy(m_Position, buffer, length);
			m_Position += length;
		}

		void Skip(size_t offset)
		{
			m_Position += offset;
		}

		NODISCARD constexpr const u8* const BufferStart() const { return m_Buffer; }
		NODISCARD constexpr const u8* const BufferEnd() const { return &m_Buffer[m_BufferSize]; }
		NODISCARD constexpr const u8* const Position() const { return m_Position; }
		NODISCARD constexpr const size_t Offset() const { return m_Position - m_Buffer; }

	private:
		u8* const m_Buffer;
		u8* m_Position;
		size m_BufferSize;
	};
}