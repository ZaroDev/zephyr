#pragma once
#include <Zephyr.h>

#include "Panels/Panel.h"

#include <Zephyr/Asset/TextureImporter.h>
#include <Zephyr/Asset/ModelImporter.h>

#include <Zephyr/ECS/Scene.h>

namespace Editor
{
	class Application final : public Zephyr::Application
	{
	public:
		Application(const Zephyr::ApplicationSpecification& spec) : Zephyr::Application(spec) {}


		NODISCARD Zephyr::Ref<Zephyr::ECS::Scene> GetActiveScene() const override { return m_Scene; }
	protected:
		void OnInit() override;
		void OnUpdate() override;
		void OnImGuiUpdate() override;
		void OnShutdown() override;

	private:
		void DockSpace();
		void MainMenuBar();
		void SetImGuiTheme();
	private:
		std::vector<Zephyr::Scope<Panel>> m_Panels;

		std::vector<Zephyr::Ref<Zephyr::Texture2D>> m_Textures;
		Zephyr::Ref<Zephyr::Model> m_Model;

		Zephyr::Ref<Zephyr::ECS::Scene> m_Scene;
	};
}