#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>
#include <Zephyr/Renderer/RenderHardwareInterface.h>
#include <Zephyr/Renderer/Shader.h>
namespace Zephyr
{
	class Renderer final
	{
	public:
		Renderer(GraphicsAPI api);
		~Renderer();

		bool Init();
		void Shutdown();

		void OnResize(i32 width, i32 height);
		void Render();

		RenderHardwareInterface& HardwareInterface() const { return  *m_GraphicsInterface; }
		ShaderLibrary& Shaders() { return m_Library; }


		static GraphicsAPI API() { return s_API; }

	private:
		static GraphicsAPI s_API;
		Scope<RenderHardwareInterface> m_GraphicsInterface;
		ShaderLibrary m_Library;
	};
}