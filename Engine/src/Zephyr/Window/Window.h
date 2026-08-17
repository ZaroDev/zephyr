#pragma once

#include <Zephyr/Modules/IModule.h>

struct GLFWwindow;
namespace Zephyr
{
	struct WindowParams
	{
		String Title = "Window";

		u32 Width = 1920;
		u32 Height = 1080;

		bool Vsync = false;
		bool Fullscreen = false;
	};

	class Window final : public IModule
	{
	public:
		Window(const WindowParams& params);
		~Window() = default;

		DEFAULT_MOVE_AND_COPY(Window);


		virtual bool Initialize() override;
		virtual void Shutdown() override;

		virtual String GetName() const override { return "Window"; };
		virtual i32 GetPriority() const override { return 0; };
		virtual UpdateFlags GetUpdateFlags() const override { return UpdateFlags::RendererUpdate; };

		GLFWwindow* GetGLFWHandle() { return m_WindowHandle; }
		bool IsOpen() const;


		virtual void PostRenderer(float deltaTime) override;

	private:
		GLFWwindow* m_WindowHandle = nullptr;
		WindowParams m_Params = {};
	};
}