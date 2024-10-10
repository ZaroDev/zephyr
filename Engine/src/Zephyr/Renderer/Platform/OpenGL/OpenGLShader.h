#pragma once

#include "OpenGLCommon.h"

#include <Zephyr/Renderer/Shader.h>

namespace Zephyr::OpenGL
{
	const Path c_ShaderPath = "Shaders\\OpenGL";

	class OpenGLShader final : public Shader
	{
	public:
		OpenGLShader(const Path& filePath);
		void Bind() override;
		void UnBind() override;

		NODISCARD u32 Program() const { return m_ProgramID; }

	private:
		u32 m_ProgramID = 0;
	};
}