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
		for (auto [_, module] : m_Modules)
		{
			module->Shutdown();
		}
		for (auto [_, module] : m_Modules)
		{
			module.reset();
		}
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