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
#include <Zephyr/Renderer/DeviceManager.h>

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
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		ApplicationCommandLineArgs Args = {};
		String Name = {};
		Path WorkingDir = {};
		GraphicsAPI GraphicsBackend = GraphicsAPI::VULKAN;
		DeviceCreationParameters DeviceParams = {};
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

		virtual void OnResize(u32 width, u32 height) ;

		NODISCARD static Application& Get() { return *s_Instance; }
		NODISCARD const ApplicationSpecification& Specification() const { return m_Specification; }
		NODISCARD virtual Ref<ECS::Scene> GetActiveScene() const = 0;

		NODISCARD DeviceManager& GetDeviceManager() const { return *m_DeviceManager; }

	protected:
		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnImGuiUpdate() = 0;
		virtual void OnShutdown() = 0;

		void LoadConfig();
		void SaveConfig();

		ApplicationSpecification m_Specification;
		bool m_Running = false;

		DeviceManager* m_DeviceManager = nullptr;

		static Application* s_Instance;
	};

	Application* CreateApplication(const ApplicationCommandLineArgs& args);
}
