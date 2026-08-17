/*
MIT License

Copyright (c) 2025 ZaroDev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <Zephyr/Core/Assert.h>
#include <Zephyr/Modules/ModuleManager.h>

namespace Zephyr
{
	namespace ECS
	{
		class Scene;
	}
	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](const int index) const
		{
			CORE_ASSERT(index < Count);
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		ApplicationCommandLineArgs Args = {};
		String Name = {};
		Path WorkingDir = {};
	};

	
	class Application
	{
	public:
		Application(const ApplicationSpecification& specs);
		virtual ~Application() = default;

		DEFAULT_MOVE_AND_COPY(Application)

		void Run();
		void Close();
		void RequestClose() { m_Running = false; }

		static Application& Get() { return *s_Instance; }
		
		template<typename T>
		Ref<T> GetModule()
		{
			return Get().m_ModuleManager.GetModule<T>();
		}

		virtual Ref<ECS::Scene> GetActiveScene() = 0;

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
	protected:
		virtual bool OnInit() { return true; }
		virtual void OnShutdown() {}

		virtual void OnPreUpdate(float deltaTime) {}
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnPostUpdate(float deltaTime) {}
					 
		virtual void OnPrePhysicsUpdate(float deltaTime) {}
		virtual void OnPhysicsUpdate(float deltaTime) {}
		virtual void OnPostPhysicsUpdate(float deltaTime) {}
					 
		virtual void OnPreRender(float deltaTime) {}
		virtual void OnRender(float deltaTime) {}
		virtual void OnImGui(float deltaTime){}
		virtual void OnPostRenderer(float deltaTime) {}

		ApplicationSpecification m_Specification;
		bool m_Running = false;

		static Application* s_Instance;

		ModuleManager m_ModuleManager;

	private:
		void PreUpdate(float deltaTime);
		void Update(float deltaTime);
		void PostUpdate(float deltaTime);

		void PrePhysics(float deltaTime);
		void Physics(float deltaTime);
		void PostPhysics(float deltaTime);

		void PreRender(float deltaTime);
		void Render(float deltaTime);
		void PostRenderer(float deltaTime);

	};

	Application* CreateApplication(const ApplicationCommandLineArgs& args);
}
