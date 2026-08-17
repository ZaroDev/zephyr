#pragma once

#include <typeindex>
#include <unordered_map>

#include "IModule.h"

namespace Zephyr
{
	class ModuleManager final
	{
	public:
		ModuleManager() = default;
		~ModuleManager();

		DISABLE_MOVE_AND_COPY(ModuleManager);

		void Initialize();
		void Shutdown();

		template<typename T, typename... Args>
		Ref<T> RegisterModule(Args&&... args)
		{
			Ref<T> newModule = CreateRef<T>(std::forward<Args>(args)...);
			m_Modules[typeid(T)] = newModule;
			if (newModule->Initialize())
			{
				CORE_INFO("Succesfully registered module {0}", newModule->GetName());
			}
			else
			{
				CORE_ERROR("Failed to initialize module {0}", newModule->GetName());
				CORE_ASSERT(!newModule->IsCoreModule(), "Failed to initialize core module!");
			}

			return newModule;
		}

		template<typename T>
		Ref<T> GetModule()
		{
			return DynCast<T>(m_Modules[typeid(T)]);
		}

		void SortModules();

		void PreUpdate(float deltaTime);
		void Update(float deltaTime);
		void PostUpdate(float deltaTime);

		void PrePhysics(float deltaTime);
		void Physics(float deltaTime);
		void PostPhysics(float deltaTime);

		void PreRender(float deltaTime);
		void Render(float deltaTime);
		void PostRenderer(float deltaTime);

	private:
		std::unordered_map<std::type_index, Ref<IModule>> m_Modules;
		std::vector<Ref<IModule>> m_UpdateList;
	};
}