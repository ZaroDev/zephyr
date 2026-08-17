#pragma once
#include <Zephyr.h>

#include "Panels/Panel.h"


#include <Zephyr/ECS/Scene.h>

namespace Editor
{
	class Application final : public Zephyr::Application
	{
	public:
		Application(const Zephyr::ApplicationSpecification& spec) : Zephyr::Application(spec) {}

		virtual Ref<ECS::Scene> GetActiveScene() override { return m_Scene; }
	protected:
		virtual bool OnInit() override;
		virtual void OnUpdate(float deltaTime) override;
		virtual void OnImGui(float deltaTime) override;
		virtual void OnShutdown() override;

	private:
		void DockSpace();
		void MainMenuBar();
		void SetImGuiTheme();
	private:
		std::vector<Zephyr::Scope<Panel>> m_Panels;
		Zephyr::Ref<Zephyr::ECS::Scene> m_Scene;
	};
}