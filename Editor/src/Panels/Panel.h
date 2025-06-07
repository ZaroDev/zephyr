#pragma once
#include <string>
#include <Zephyr/Core/Base.h>
#include <FontIcons/IconsForkAwesome.h>

namespace Editor
{
	enum class PanelCategory : std::uint8_t
	{
		WINDOW = BIT(0),
		INFO = BIT(1),
	};
	using namespace Zephyr;
	class Panel
	{
	public:
		Panel(Zephyr::StrView name, PanelCategory category)
			: m_Name(name), m_Category(category) {}

		virtual ~Panel() = default;

		DEFAULT_MOVE_AND_COPY(Panel);

		virtual void OnUpdate() = 0;
		virtual void OnImGui() = 0;

		NODISCARD PanelCategory GetCategory() const { return m_Category; }
		NODISCARD Zephyr::StrView GetName() const { return m_Name; }
		NODISCARD bool IsActive() const { return m_Open; }
		void SwitchActive()
		{
			m_Open = !m_Open;
		}

	protected:
		Zephyr::String m_Name;
		bool m_Open = true;
		PanelCategory m_Category;
	};
}