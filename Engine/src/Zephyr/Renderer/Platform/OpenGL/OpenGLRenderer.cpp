#include <pch.h>
#include "OpenGLRenderer.h"

#include <GLFW/glfw3.h>

#include <ImGui/imgui_impl_opengl3.h>
#include <ImGui/imgui_impl_glfw.h>

namespace Zephyr::OpenGL::Core
{
	bool Init()
	{
		glfwMakeContextCurrent(Window::GetGLFWWindow());
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
		{
			CORE_ERROR("Failed to load glad: GLFW proc address!");
			return false;
		}

		return true;
	}
	void Shutdown()
	{

	}
	void OnResize(i32 width, i32 height)
	{
		glViewport(0, 0, width, height);
	}
	void BeginFrame(Camera& camera)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	}
	void EndFrame()
	{

	}
	//void CreateTexture(D3D11Texture2D& texture, Buffer buffer = Buffer());


	bool InitImGui()
	{
		ImGui_ImplGlfw_InitForOther(Window::GetGLFWWindow(), true);
		ImGui_ImplOpenGL3_Init("#version 460");
		return true;
	}
	void ImGuiNewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}
	void ImGuiEndFrame()
	{
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
	void ShutdownImGui()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
	}

	void CreateVertexBuffer()
	{

	}
}