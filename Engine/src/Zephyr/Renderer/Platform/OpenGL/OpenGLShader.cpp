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

		m_ProgramID = glCreateProgram();

		const u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		const char* cVertSrc = vertexShaderSource.c_str();
		glShaderSource(vertexShader, 1, &cVertSrc, NULL);
		glCompileShader(vertexShader);

		glAttachShader(m_ProgramID, vertexShader);
		glDeleteShader(vertexShader);

		const char* cFragSrc = fragmentShaderSource.c_str();
		glShaderSource(fragmentShader, 1, &cFragSrc, NULL);
		glCompileShader(fragmentShader);

		glAttachShader(m_ProgramID, fragmentShader);
		glDeleteShader(fragmentShader);
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