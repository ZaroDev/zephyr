#pragma once

namespace Zephyr
{

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](const int index) const
		{
			
			return Args[index];
		}
	};


	class Application
	{
	public:

		void Run();

		void Close();

	private:

	};

	Application& CreateApplication(const ApplicationCommandLineArgs& args);
}