#pragma once
#include <Zephyr/Core/Application.h>
#include <Zephyr/Core/Log.h>

extern Zephyr::Application* Zephyr::CreateApplication(const ApplicationCommandLineArgs& args);

namespace Zephyr
{
	int EntryPoint(int argc, char** argv)
	{
		Log::Init();

		const auto app = Zephyr::CreateApplication({ argc, argv });

		app->Run();

		app->Close();

		return 0;
	}
}
#if defined(PLATFORM_WINDOWS) && defined(DIST)
#include <shellapi.h>
#include <atlstr.h>

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdLine, int cmdShow)
{
	int argc = 0;
	LPWSTR* argvW;

	argvW = CommandLineToArgvW(GetCommandLine(), &argc);

	char** argv = nullptr;
	for (u32 i = 0; i < argc; i++)
	{
		argv[i] = CW2A(argvW[i]);
	}

	return Zephyr::EntryPoint(argc, argv);
}

#else
int main(int argc, char** argv)
{
	return Zephyr::EntryPoint(argc, argv);
}
#endif