#pragma once
#include "Panel.h"

#include <Zephyr/Core/Base.h>
#include <Zephyr/Math/MathTypes.h>
#include <Zephyr/Renderer/Framebuffer.h>


namespace Editor
{
	class ScenePanel final : public Panel
	{
	public:
		ScenePanel();
		~ScenePanel() override;

		void OnUpdate() override;
		void OnImGui() override;

	private:
		Zephyr::Ref<Zephyr::Framebuffer> m_ViewPort;

		Zephyr::Iv2 m_LastViewportSize;

		u32 m_SelectedBuffer;
	};
}