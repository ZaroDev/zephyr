#include <pch.h>

#include "WindowsPlatformUtils.h"

#include "Core/Application.h"
#ifdef PLATFORM_WINDOWS

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Zephyr/Renderer/Window.h>

namespace Zephyr::FileDialogs::Windows
{
	Zephyr::Path OpenFile(Zephyr::StrView filter)
	{
		OPENFILENAMEA ofn; //common dialog box structure
		CHAR szFile[260] = { 0 }; // if using TCHAR macros

		//Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(Application::Get().GetWindow().GetGLFWWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return Path(ofn.lpstrFile);
		}
		return Path();
	}
	Zephyr::Path SaveFile(Zephyr::StrView filter)
	{
		OPENFILENAMEA ofn; //common dialog box structure
		CHAR szFile[260] = { 0 }; // if using TCHAR macros

		//Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(Application::Get().GetWindow().GetGLFWWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return Path(ofn.lpstrFile);
		}
		return std::string();
	}
}

#endif