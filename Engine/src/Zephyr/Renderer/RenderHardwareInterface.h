#pragma once
#include <Zephyr/Math/MathTypes.h>
#include <Zephyr/Renderer/Window.h>
namespace Zephyr
{
	class RenderHardwareInterface
	{
	public:
		RenderHardwareInterface() = default;
		virtual ~RenderHardwareInterface() = default;
		virtual bool Init() = 0;
		virtual void Shutdown() = 0;
		virtual void OnResize(i32 width, i32 height) = 0;
		virtual void Render() = 0;


		static Scope<RenderHardwareInterface> Create(GraphicsAPI api);
	};
}