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
#include <Zephyr/Renderer/IRenderPass.h>
#include <Zephyr/Renderer/ShaderFactory.h>

#include <imgui.h>
namespace Zephyr
{
    // From donut
    struct ImGui_NVRHI
    {
        nvrhi::DeviceHandle m_device;
        nvrhi::CommandListHandle m_commandList;

        nvrhi::ShaderHandle vertexShader;
        nvrhi::ShaderHandle pixelShader;
        nvrhi::InputLayoutHandle shaderAttribLayout;

        nvrhi::TextureHandle fontTexture;
        nvrhi::SamplerHandle fontSampler;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;

        nvrhi::BindingLayoutHandle bindingLayout;
        nvrhi::GraphicsPipelineDesc basePSODesc;

        nvrhi::GraphicsPipelineHandle pso;
        std::unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> bindingsCache;

        std::vector<ImDrawVert> vtxBuffer;
        std::vector<ImDrawIdx> idxBuffer;

        bool Init(nvrhi::IDevice* device, Ref<ShaderFactory> shaderFactory);
        bool UpdateFontTexture();
        bool Render(nvrhi::IFramebuffer* framebuffer);
        void BackbufferResizing();

    private:
        bool ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer);


        nvrhi::IGraphicsPipeline* GetPSO(nvrhi::IFramebuffer* fb);
        nvrhi::IBindingSet* GetBindingSet(nvrhi::ITexture* texture);
        bool UpdateGeometry(nvrhi::ICommandList* commandList);
    };

    class RegisteredFont
    {
    protected:
        friend class ImGuiRenderPass;

        Ref<IBlob> m_data;
        bool const m_isDefault;
        bool const m_isCompressed;
        float const m_sizeAtDefaultScale;
        ImFont* m_imFont = nullptr;

        void CreateScaledFont(float displayScale);
        void ReleaseScaledFont();
    public:
        // Creates an invalid font that will not add any ImGUI fonts
        RegisteredFont()
            : m_isDefault(false)
            , m_isCompressed(false)
            , m_sizeAtDefaultScale(0.f)
        {
        }

        // Creates a default font with the given size
        RegisteredFont(float size)
            : m_isDefault(true)
            , m_isCompressed(false)
            , m_sizeAtDefaultScale(size)
        {
        }

        // Creates a custom font
        RegisteredFont(Ref<IBlob> data, bool isCompressed, float size)
            : m_data(data)
            , m_isDefault(false)
            , m_isCompressed(isCompressed)
            , m_sizeAtDefaultScale(size)
        {
        }

        // Returns true if the custom font data has been successfully loaded.
        // This doesn't necessarily mean that the font data is valid: the actual font object is only created
        // in the first call to ImGui_Renderer::Animate(...). After that, use GetScaledFont()
        // to test if the font is valid.
        bool HasFontData() const { return m_data != nullptr; }

        // Returns the ImFont object that can be used with ImGUI.
        // Note that the returned pointer is transient and will change when screen DPI changes,
        // or when new fonts are loaded. Do not cache the returned value between frames.
        // The returned pointer may be NULL if the font has failed to load, which is OK for ImGUI's PushFont(...)
        ImFont* GetScaledFont() { return m_imFont; }
    };

	class ImGuiRenderPass : public IRenderPass
	{
    public:
        ImGuiRenderPass(DeviceManager* devManager);
        ~ImGuiRenderPass();
        bool Init(std::shared_ptr<ShaderFactory> shaderFactory);

        // Loads a TTF font from file and registers it with the ImGui_Renderer.
        // To use the font with ImGUI at runtime, call RegisteredFont::GetScaledFont().
        std::shared_ptr<RegisteredFont> CreateFontFromFile(IFileSystem& fs, Path const& fontFile, float fontSize);

        // Registers a TTF font stored in memory with the ImGui_Renderer.
        // To use the font with ImGUI at runtime, call RegisteredFont::GetScaledFont().
        std::shared_ptr<RegisteredFont> CreateFontFromMemory(void const* pData, size_t size, float fontSize);

        // Identical to CreateFontFromMemory except that the data is compressed
        // using 'binary_to_compressed_c.cpp' in imgui.
        std::shared_ptr<RegisteredFont> CreateFontFromMemoryCompressed(void const* pData, size_t size, float fontSize);

        // Returns the default font.
        std::shared_ptr<RegisteredFont> GetDefaultFont() { return m_defaultFont; }

        virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) override;
        virtual bool KeyboardCharInput(unsigned int unicode, int mods) override;
        virtual bool MousePosUpdate(double xpos, double ypos) override;
        virtual bool MouseScrollUpdate(double xoffset, double yoffset) override;
        virtual bool MouseButtonUpdate(int button, int action, int mods) override;
        virtual void Animate(float elapsedTimeSeconds) override;
        virtual void Render(nvrhi::IFramebuffer* framebuffer) override;
        virtual void BackBufferResizing() override;
        virtual void DisplayScaleChanged(float scaleX, float scaleY) override;
    protected:

        Scope<ImGui_NVRHI> imgui_nvrhi;

        // buffer mouse click and keypress events to make sure we don't lose events which last less than a full frame
        std::array<bool, 3> mouseDown = { false };
        std::array<bool, GLFW_KEY_LAST + 1> keyDown = { false };

        std::vector<Ref<RegisteredFont>> m_fonts;

        Ref<RegisteredFont> m_defaultFont;

        bool m_supportExplicitDisplayScaling;
        bool m_beginFrameCalled = false;

    protected:
        // creates the UI in ImGui, updates internal UI state
        virtual void buildUI(void);

        void BeginFullScreenWindow();
        void DrawScreenCenteredText(const char* text);
        void EndFullScreenWindow();
    private:
        std::shared_ptr<RegisteredFont> CreateFontFromMemoryInternal(void const* pData, size_t size,
            bool compressed, float fontSize);
	};
}
