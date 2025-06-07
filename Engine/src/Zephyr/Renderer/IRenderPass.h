#pragma once
#include <Zephyr/Renderer/DeviceManager.h>

#include <nvrhi/nvrhi.h>
namespace Zephyr
{
	class IRenderPass
	{ 
    private:
        DeviceManager* m_DeviceManager;

    public:
        explicit IRenderPass(DeviceManager* deviceManager)
            : m_DeviceManager(deviceManager)
        {
        }

        virtual ~IRenderPass() = default;

        virtual void SetLatewarpOptions() {}
        virtual bool ShouldRenderUnfocused() { return false; }
        virtual void Render(nvrhi::IFramebuffer* framebuffer) {}
        virtual void Animate(float fElapsedTimeSeconds) {}
        virtual void BackBufferResizing() {}
        virtual void BackBufferResized(const uint32_t width, const uint32_t height, const uint32_t sampleCount) {}

        // Called before Animate() when a DPI change was detected
        virtual void DisplayScaleChanged(float scaleX, float scaleY) {}

        // all of these pass in GLFW constants as arguments
        // see http://www.glfw.org/docs/latest/input.html
        // return value is true if the event was consumed by this render pass, false if it should be passed on
        virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) { return false; }
        virtual bool KeyboardCharInput(unsigned int unicode, int mods) { return false; }
        virtual bool MousePosUpdate(double xpos, double ypos) { return false; }
        virtual bool MouseScrollUpdate(double xoffset, double yoffset) { return false; }
        virtual bool MouseButtonUpdate(int button, int action, int mods) { return false; }
        virtual bool JoystickButtonUpdate(int button, bool pressed) { return false; }
        virtual bool JoystickAxisUpdate(int axis, float value) { return false; }

        NODISCARD DeviceManager* GetDeviceManager() const { return m_DeviceManager; }
        NODISCARD nvrhi::IDevice* GetDevice() const { return m_DeviceManager->GetDevice(); }
        NODISCARD uint32_t GetFrameIndex() const { return m_DeviceManager->GetFrameIndex(); }
	};
}