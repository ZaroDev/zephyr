#pragma once

#include <Zephyr/Core/Base.h>
#include <Zephyr/Core/Application.h>


extern Zephyr::Application& Zephyr::CreateApplication(const ApplicationCommandLineArgs& args);

namespace Zephyr
{
	int EntryPoint(int argc, char** argv)
	{
		auto app = Zephyr::CreateApplication({ argc, argv });

		app.Run();

		app.Close();

		return 0;
	}
}