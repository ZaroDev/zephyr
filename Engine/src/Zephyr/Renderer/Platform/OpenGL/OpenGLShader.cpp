#include <pch.h>
#include "OpenGLShader.h"

#include <Zephyr/FileSystem/FileSystem.h>

namespace Zephyr::OpenGL
{
	OpenGLShader::OpenGLShader(const Path& filePath)
	{
		m_Name = filePath.string();
		Path file = c_ShaderPath;
		file /= filePath;

		const String vertexShaderSource = FileSystem::ReadFileContents(FileSystem::ReplaceExtension(file, "vs"));
		const String fragmentShaderSource = FileSystem::ReadFileContents(FileSystem::ReplaceExtension(file, "fs"));


	}
	void OpenGLShader::Bind()
	{
		glUseProgram(m_ProgramID);
	}
	void OpenGLShader::UnBind()
	{
		glUseProgram(0);
	}
}