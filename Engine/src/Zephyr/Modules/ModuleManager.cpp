#include <pch.h>
#include "ModuleManager.h"

namespace Zephyr
{
	ModuleManager::~ModuleManager()
	{
		Shutdown();
	}

	void ModuleManager::Initialize()
	{
		SortModules();
	}

	void ModuleManager::Shutdown()
	{
		// Explicit Shutdown() calls must run in reverse-priority order (highest priority first),
		// not m_Modules' unordered_map order - e.g. Renderer (priority 1) owns an ImGuiRenderer
		// whose destruction path touches the GLFW window via imgui_impl_glfw, which must happen
		// before Window (priority 0) calls glfwDestroyWindow()/glfwTerminate() in its own
		// Shutdown(). SortModules() is re-run here in case Shutdown() is called before any Run()
		// loop iteration has populated m_UpdateList.
		SortModules();
		for (auto it = m_UpdateList.rbegin(); it != m_UpdateList.rend(); ++it)
		{
			(*it)->Shutdown();
		}
		m_UpdateList.clear();
		m_Modules.clear();
	}

	void ModuleManager::SortModules()
	{
		m_UpdateList.clear();
		for (auto [_, module] : m_Modules)
		{
			m_UpdateList.push_back(module);
		}

		std::sort(m_UpdateList.begin(), m_UpdateList.end(), [](const auto& a, const auto& b)
			{
				return a->GetPriority() < b->GetPriority();
			});
	}

	void ModuleManager::PreUpdate(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::Update))
			{
				module->PreUpdate(deltaTime);
			}
		}
	}

	void ModuleManager::Update(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::Update))
			{
				module->Update(deltaTime);
			}
		}
	}

	void ModuleManager::PostUpdate(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::Update))
			{
				module->PostUpdate(deltaTime);
			}
		}
	}

	void ModuleManager::PrePhysics(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::PhysicsUpdate))
			{
				module->PrePhysicsUpdate(deltaTime);
			}
		}
	}

	void ModuleManager::Physics(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::PhysicsUpdate))
			{
				module->PhysicsUpdate(deltaTime);
			}
		}
	}

	void ModuleManager::PostPhysics(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::PhysicsUpdate))
			{
				module->PostPhysicsUpdate(deltaTime);
			}
		}
	}

	void ModuleManager::PreRender(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::RendererUpdate))
			{
				module->PreRender(deltaTime);
			}
		}
	}

	void ModuleManager::Render(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::RendererUpdate))
			{
				module->Render(deltaTime);
			}
		}
	}

	void ModuleManager::PostRenderer(float deltaTime)
	{
		for (Ref<IModule>& module : m_UpdateList)
		{
			if (Any(module->GetUpdateFlags(), UpdateFlags::RendererUpdate))
			{
				module->PostRenderer(deltaTime);
			}
		}
	}
}