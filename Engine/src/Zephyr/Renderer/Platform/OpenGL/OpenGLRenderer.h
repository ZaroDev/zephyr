#pragma once

#include <Zephyr/Renderer/RenderHardwareInterface.h>

namespace Zephyr::OpenGL
{
	class OpenGLRenderer final : public RenderHardwareInterface
	{
	public:
		OpenGLRenderer() : RenderHardwareInterface() {}
		~OpenGLRenderer() = default;

		bool Init() override;
		void Shutdown() override;
		void OnResize(i32 width, i32 height) override;
		void Render() override;
	};
}