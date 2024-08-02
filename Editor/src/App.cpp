#include <Zephyr.h>

namespace Zephyr
{
	class Editor final : public Application
	{
	public:
		Editor(const ApplicationSpecification& spec) : Application(spec) {}
	};


	Application* CreateApplication(const ApplicationCommandLineArgs& args)
	{
		ApplicationSpecification spec;
		spec.Args = args;

#ifdef PLATFORM_WINDOWS
		spec.API = GraphicsAPI::DX11;
#else
		spec.API = GraphicsAPI::OPENGL;
#endif
		spec.Name = "Zephyr Editor";

		return new Editor(spec);
	}
}