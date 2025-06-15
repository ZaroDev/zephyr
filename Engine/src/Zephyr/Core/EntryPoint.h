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