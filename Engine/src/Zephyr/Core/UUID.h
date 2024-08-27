#pragma once

namespace Zephyr
{
	class UUID
	{
	public:
		UUID();
		UUID(u64 uuid);
		UUID(const UUID&) = default;

		operator u64() const { return m_UUID; }
	private:
		u64 m_UUID;
	};
}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Zephyr::UUID>
	{
		std::size_t operator()(const Zephyr::UUID& uuid) const
		{
			return (u64)uuid;
		}
	};

}