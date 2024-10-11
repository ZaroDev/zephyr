#pragma once
#include <Zephyr.h>

#include "Panels/Panel.h"

#include <Zephyr/Asset/TextureImporter.h>


namespace Editor
{
	class Application final : public Zephyr::Application
	{
	public:
		Application(const Zephyr::ApplicationSpecification& spec) : Zephyr::Application(spec) {}

	protected:
		void OnInit() override;
		void OnUpdate() override;
		void OnImGuiUpdate() override;
		void OnShutdown() override;

	private:
		void DockSpace();
		void MainMenuBar();

	private:
		std::vector<Zephyr::Scope<Panel>> m_Panels;

		std::vector<Zephyr::Ref<Zephyr::Texture2D>> m_Textures;
	};
}