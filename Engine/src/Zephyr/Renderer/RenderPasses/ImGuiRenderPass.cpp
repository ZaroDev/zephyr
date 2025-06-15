#include <pch.h>
#include "ImGuiRenderPass.h"

namespace Zephyr
{
    struct VERTEX_CONSTANT_BUFFER
    {
        float        mvp[4][4];
    };

    bool ImGui_NVRHI::UpdateFontTexture()
    {
        ImGuiIO& io = ImGui::GetIO();

        // If the font texture exists and is bound to ImGui, we're done.
        // Note: ImGui_Renderer will reset io.Fonts->TexID when new fonts are added.
        if (fontTexture && io.Fonts->TexID)
            return true;

        unsigned char* pixels;
        int width, height;

        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        if (!pixels)
            return false;

        nvrhi::TextureDesc textureDesc;
        textureDesc.width = width;
        textureDesc.height = height;
        textureDesc.format = nvrhi::Format::RGBA8_UNORM;
        textureDesc.debugName = "ImGui font texture";

        fontTexture = m_device->createTexture(textureDesc);

        if (fontTexture == nullptr)
            return false;

        m_commandList->open();

        m_commandList->beginTrackingTextureState(fontTexture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);

        m_commandList->writeTexture(fontTexture, 0, 0, pixels, width * 4);

        m_commandList->setPermanentTextureState(fontTexture, nvrhi::ResourceStates::ShaderResource);
        m_commandList->commitBarriers();

        m_commandList->close();
        m_device->executeCommandList(m_commandList);

        //io.Fonts->TexID = fontTexture;

        return true;
    }

    bool ImGui_NVRHI::Init(nvrhi::IDevice* device, std::shared_ptr<ShaderFactory> shaderFactory)
    {
        m_device = device;

        m_commandList = m_device->createCommandList();

        vertexShader = shaderFactory->CreateShader("imgui_vertex", "main", nullptr, nvrhi::ShaderType::Vertex);
        pixelShader = shaderFactory->CreateShader("imgui_pixel", "main", nullptr, nvrhi::ShaderType::Pixel);

        if (!vertexShader || !pixelShader)
        {
            CORE_ERROR("Failed to create an ImGUI shader");
            return false;
        }

        // create attribute layout object
        nvrhi::VertexAttributeDesc vertexAttribLayout[] = {
            { "POSITION", nvrhi::Format::RG32_FLOAT,  1, 0, offsetof(ImDrawVert,pos), sizeof(ImDrawVert), false },
            { "TEXCOORD", nvrhi::Format::RG32_FLOAT,  1, 0, offsetof(ImDrawVert,uv),  sizeof(ImDrawVert), false },
            { "COLOR",    nvrhi::Format::RGBA8_UNORM, 1, 0, offsetof(ImDrawVert,col), sizeof(ImDrawVert), false },
        };

        shaderAttribLayout = m_device->createInputLayout(vertexAttribLayout, sizeof(vertexAttribLayout) / sizeof(vertexAttribLayout[0]), vertexShader);

        // create PSO
        {
            nvrhi::BlendState blendState;
            blendState.targets[0].setBlendEnable(true)
                .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
                .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
                .setSrcBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha)
                .setDestBlendAlpha(nvrhi::BlendFactor::Zero);

            auto rasterState = nvrhi::RasterState()
                .setFillSolid()
                .setCullNone()
                .setScissorEnable(true)
                .setDepthClipEnable(true);

            auto depthStencilState = nvrhi::DepthStencilState()
                .disableDepthTest()
                .enableDepthWrite()
                .disableStencil()
                .setDepthFunc(nvrhi::ComparisonFunc::Always);

            nvrhi::RenderState renderState;
            renderState.blendState = blendState;
            renderState.depthStencilState = depthStencilState;
            renderState.rasterState = rasterState;

            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::All;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::PushConstants(0, sizeof(float) * 2),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Sampler(0)
            };
            bindingLayout = m_device->createBindingLayout(layoutDesc);

            basePSODesc.primType = nvrhi::PrimitiveType::TriangleList;
            basePSODesc.inputLayout = shaderAttribLayout;
            basePSODesc.VS = vertexShader;
            basePSODesc.PS = pixelShader;
            basePSODesc.renderState = renderState;
            basePSODesc.bindingLayouts = { bindingLayout };
        }

        {
            const auto desc = nvrhi::SamplerDesc()
                .setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
                .setAllFilters(true);

            fontSampler = m_device->createSampler(desc);

            if (fontSampler == nullptr)
                return false;
        }

        return true;
    }

    bool ImGui_NVRHI::ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, const bool indexBuffer)
    {
        if (buffer == nullptr || size_t(buffer->getDesc().byteSize) < requiredSize)
        {
            nvrhi::BufferDesc desc;
            desc.byteSize = uint32_t(reallocateSize);
            desc.structStride = 0;
            desc.debugName = indexBuffer ? "ImGui index buffer" : "ImGui vertex buffer";
            desc.canHaveUAVs = false;
            desc.isVertexBuffer = !indexBuffer;
            desc.isIndexBuffer = indexBuffer;
            desc.isDrawIndirectArgs = false;
            desc.isVolatile = false;
            desc.initialState = indexBuffer ? nvrhi::ResourceStates::IndexBuffer : nvrhi::ResourceStates::VertexBuffer;
            desc.keepInitialState = true;

            buffer = m_device->createBuffer(desc);

            if (!buffer)
            {
                return false;
            }
        }

        return true;
    }

    nvrhi::IGraphicsPipeline* ImGui_NVRHI::GetPSO(nvrhi::IFramebuffer* fb)
    {
        if (pso)
            return pso;

        pso = m_device->createGraphicsPipeline(basePSODesc, fb);
        assert(pso);

        return pso;
    }

    nvrhi::IBindingSet* ImGui_NVRHI::GetBindingSet(nvrhi::ITexture* texture)
    {
        auto iter = bindingsCache.find(texture);
        if (iter != bindingsCache.end())
        {
            return iter->second;
        }

        nvrhi::BindingSetDesc desc;

        desc.bindings = {
            nvrhi::BindingSetItem::PushConstants(0, sizeof(float) * 2),
            nvrhi::BindingSetItem::Texture_SRV(0, texture),
            nvrhi::BindingSetItem::Sampler(0, fontSampler)
        };

        nvrhi::BindingSetHandle binding;
        binding = m_device->createBindingSet(desc, bindingLayout);
        assert(binding);

        bindingsCache[texture] = binding;
        return binding;
    }

    bool ImGui_NVRHI::UpdateGeometry(nvrhi::ICommandList* commandList)
    {
        ImDrawData* drawData = ImGui::GetDrawData();

        // create/resize vertex and index buffers if needed
        if (!ReallocateBuffer(vertexBuffer,
            drawData->TotalVtxCount * sizeof(ImDrawVert),
            (drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert),
            false))
        {
            return false;
        }

        if (!ReallocateBuffer(indexBuffer,
            drawData->TotalIdxCount * sizeof(ImDrawIdx),
            (drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx),
            true))
        {
            return false;
        }

        vtxBuffer.resize(vertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
        idxBuffer.resize(indexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

        // copy and convert all vertices into a single contiguous buffer
        ImDrawVert* vtxDst = &vtxBuffer[0];
        ImDrawIdx* idxDst = &idxBuffer[0];

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];

            memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

            vtxDst += cmdList->VtxBuffer.Size;
            idxDst += cmdList->IdxBuffer.Size;
        }

        commandList->writeBuffer(vertexBuffer, &vtxBuffer[0], vertexBuffer->getDesc().byteSize);
        commandList->writeBuffer(indexBuffer, &idxBuffer[0], indexBuffer->getDesc().byteSize);

        return true;
    }


    bool ImGui_NVRHI::Render(nvrhi::IFramebuffer* framebuffer)
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        const auto& io = ImGui::GetIO();

        m_commandList->open();
        m_commandList->beginMarker("ImGUI");

        if (!UpdateGeometry(m_commandList))
        {
            m_commandList->close();
            return false;
        }

        // handle DPI scaling
        drawData->ScaleClipRects(io.DisplayFramebufferScale);

        float invDisplaySize[2] = { 1.f / io.DisplaySize.x, 1.f / io.DisplaySize.y };

        // set up graphics state
        nvrhi::GraphicsState drawState;

        drawState.framebuffer = framebuffer;
        assert(drawState.framebuffer);

        drawState.pipeline = GetPSO(drawState.framebuffer);

        drawState.viewport.viewports.push_back(nvrhi::Viewport(io.DisplaySize.x * io.DisplayFramebufferScale.x,
            io.DisplaySize.y * io.DisplayFramebufferScale.y));
        drawState.viewport.scissorRects.resize(1);  // updated below

        nvrhi::VertexBufferBinding vbufBinding;
        vbufBinding.buffer = vertexBuffer;
        vbufBinding.slot = 0;
        vbufBinding.offset = 0;
        drawState.vertexBuffers.push_back(vbufBinding);

        drawState.indexBuffer.buffer = indexBuffer;
        drawState.indexBuffer.format = (sizeof(ImDrawIdx) == 2 ? nvrhi::Format::R16_UINT : nvrhi::Format::R32_UINT);
        drawState.indexBuffer.offset = 0;

        // render command lists
        int vtxOffset = 0;
        int idxOffset = 0;
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            for (int i = 0; i < cmdList->CmdBuffer.Size; i++)
            {
                const ImDrawCmd* pCmd = &cmdList->CmdBuffer[i];

                if (pCmd->UserCallback)
                {
                    pCmd->UserCallback(cmdList, pCmd);
                }
                else {
                    drawState.bindings = { GetBindingSet((nvrhi::ITexture*)pCmd->TextureId) };
                    assert(drawState.bindings[0]);

                    drawState.viewport.scissorRects[0] = nvrhi::Rect(int(pCmd->ClipRect.x),
                        int(pCmd->ClipRect.z),
                        int(pCmd->ClipRect.y),
                        int(pCmd->ClipRect.w));

                    nvrhi::DrawArguments drawArguments;
                    drawArguments.vertexCount = pCmd->ElemCount;
                    drawArguments.startIndexLocation = idxOffset;
                    drawArguments.startVertexLocation = vtxOffset;

                    m_commandList->setGraphicsState(drawState);
                    m_commandList->setPushConstants(invDisplaySize, sizeof(invDisplaySize));
                    m_commandList->drawIndexed(drawArguments);
                }

                idxOffset += pCmd->ElemCount;
            }

            vtxOffset += cmdList->VtxBuffer.Size;
        }

        m_commandList->endMarker();
        m_commandList->close();
        m_device->executeCommandList(m_commandList);

        return true;
    }

    void ImGui_NVRHI::BackbufferResizing()
    {
        pso = nullptr;
    }

    ImGuiRenderPass::ImGuiRenderPass(DeviceManager* devManager)
        : IRenderPass(devManager)
        , m_supportExplicitDisplayScaling(devManager->GetDeviceParams().SupportExplicitDisplayScaling)
    {
        ImGui::CreateContext();

        m_defaultFont = std::make_shared<RegisteredFont>(13.f);
        m_fonts.push_back(m_defaultFont);
    }

    ImGuiRenderPass::~ImGuiRenderPass()
    {
        ImGui::DestroyContext();
    }

    bool ImGuiRenderPass::Init(std::shared_ptr<ShaderFactory> shaderFactory)
    {
        // Set up keyboard mapping.
        // ImGui will use those indices to peek into the io.KeyDown[] array
        // that we will update during the application lifetime.
        ImGuiIO& io = ImGui::GetIO();
      
        imgui_nvrhi = std::make_unique<ImGui_NVRHI>();
        return imgui_nvrhi->Init(GetDevice(), shaderFactory);
    }

    std::shared_ptr<RegisteredFont> ImGuiRenderPass::CreateFontFromFile(IFileSystem& fs,
        const Path& fontFile, float fontSize)
    {
        auto fontData = fs.ReadFile(fontFile);
        if (!fontData)
            return std::make_shared<RegisteredFont>();

        auto font = std::make_shared<RegisteredFont>(fontData, false, fontSize);
        m_fonts.push_back(font);

        return std::move(font);
    }

    std::shared_ptr<RegisteredFont> ImGuiRenderPass::CreateFontFromMemoryInternal(void const* pData, size_t size,
        bool compressed, float fontSize)
    {
        if (!pData || !size)
            return std::make_shared<RegisteredFont>();

        // Copy the font data into a blob to make the RegisteredFont object own it
        void* dataCopy = malloc(size);
        memcpy(dataCopy, pData, size);
        std::shared_ptr<Blob> blob = std::make_shared<Blob>(dataCopy, size);

        auto font = std::make_shared<RegisteredFont>(blob, compressed, fontSize);
        m_fonts.push_back(font);

        return std::move(font);
    }

    std::shared_ptr<RegisteredFont> ImGuiRenderPass::CreateFontFromMemory(void const* pData, size_t size, float fontSize)
    {
        return CreateFontFromMemoryInternal(pData, size, false, fontSize);
    }

    std::shared_ptr<RegisteredFont> ImGuiRenderPass::CreateFontFromMemoryCompressed(void const* pData, size_t size,
        float fontSize)
    {
        return CreateFontFromMemoryInternal(pData, size, true, fontSize);
    }

    bool ImGuiRenderPass::KeyboardUpdate(int key, int scancode, int action, int mods)
    {
        auto& io = ImGui::GetIO();

        bool keyIsDown;
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            keyIsDown = true;
        }
        else {
            keyIsDown = false;
        }

        // update our internal state tracking for this key button
        keyDown[key] = keyIsDown;

        if (keyIsDown)
        {
            // if the key was pressed, update ImGui immediately
            //io.KeysDown[key] = true;
        }
        else {
            // for key up events, ImGui state is only updated after the next frame
            // this ensures that short keypresses are not missed
        }

        return io.WantCaptureKeyboard;
    }

    bool ImGuiRenderPass::KeyboardCharInput(unsigned int unicode, int mods)
    {
        auto& io = ImGui::GetIO();

        io.AddInputCharacter(unicode);

        return io.WantCaptureKeyboard;
    }

    bool ImGuiRenderPass::MousePosUpdate(double xpos, double ypos)
    {
        auto& io = ImGui::GetIO();
        io.MousePos.x = float(xpos);
        io.MousePos.y = float(ypos);

        return io.WantCaptureMouse;
    }

    bool ImGuiRenderPass::MouseScrollUpdate(double xoffset, double yoffset)
    {
        auto& io = ImGui::GetIO();
        io.MouseWheel += float(yoffset);

        return io.WantCaptureMouse;
    }

    bool ImGuiRenderPass::MouseButtonUpdate(int button, int action, int mods)
    {
        auto& io = ImGui::GetIO();

        bool buttonIsDown;
        int buttonIndex;

        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            buttonIsDown = true;
        }
        else {
            buttonIsDown = false;
        }

        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            buttonIndex = 0;
            break;

        case GLFW_MOUSE_BUTTON_RIGHT:
            buttonIndex = 1;
            break;

        case GLFW_MOUSE_BUTTON_MIDDLE:
            buttonIndex = 2;
            break;
        }

        // update our internal state tracking for this mouse button
        mouseDown[buttonIndex] = buttonIsDown;

        if (buttonIsDown)
        {
            // update ImGui state immediately
            io.MouseDown[buttonIndex] = true;
        }
        else {
            // for mouse up events, ImGui state is only updated after the next frame
            // this ensures that short clicks are not missed
        }

        return io.WantCaptureMouse;
    }

    void ImGuiRenderPass::Animate(float elapsedTimeSeconds)
    {
        // multiple Animate may be called before the first Render due to the m_SkipRenderOnFirstFrame extension
        // ensure each imgui_nvrhi->beginFrame matches with exactly one imgui_nvrhi->Render
        if (!imgui_nvrhi || m_beginFrameCalled)
            return;

        // Make sure that all registered fonts have corresponding ImFont objects at the current DPI scale
        float scaleX, scaleY;
        GetDeviceManager()->GetDPIScaleInfo(scaleX, scaleY);
        for (auto& font : m_fonts)
        {
            if (!font->GetScaledFont())
                font->CreateScaledFont(m_supportExplicitDisplayScaling ? scaleX : 1.f);
        }

        // Creates the font texture if it's not yet valid
        imgui_nvrhi->UpdateFontTexture();

        int w, h;
        GetDeviceManager()->GetWindowDimensions(w, h);

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(float(w), float(h));
        if (!m_supportExplicitDisplayScaling)
        {
            io.DisplayFramebufferScale.x = scaleX;
            io.DisplayFramebufferScale.y = scaleY;
        }

       /* io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];*/

        io.DeltaTime = elapsedTimeSeconds;
        io.MouseDrawCursor = false;

        ImGui::NewFrame();

        m_beginFrameCalled = true;
    }

    void ImGuiRenderPass::Render(nvrhi::IFramebuffer* framebuffer)
    {
        if (!imgui_nvrhi) return;

        buildUI();

        ImGui::Render();
        imgui_nvrhi->Render(framebuffer);
        m_beginFrameCalled = false;

        // reconcile mouse button states
        auto& io = ImGui::GetIO();
        for (size_t i = 0; i < mouseDown.size(); i++)
        {
            if (io.MouseDown[i] == true && mouseDown[i] == false)
            {
                io.MouseDown[i] = false;
            }
        }

        // reconcile key states
        for (size_t i = 0; i < keyDown.size(); i++)
        {
           /* if (io.KeysDown[i] == true && keyDown[i] == false)
            {
                io.KeysDown[i] = false;
            }*/
        }
    }

    void ImGuiRenderPass::BackBufferResizing()
    {
        if (imgui_nvrhi) imgui_nvrhi->BackbufferResizing();
    }

    void ImGuiRenderPass::DisplayScaleChanged(float scaleX, float scaleY)
    {
        // Apps that don't implement explicit scaling won't expect the fonts to be resized etc.
        if (!m_supportExplicitDisplayScaling)
            return;

        auto& io = ImGui::GetIO();

        // Clear the ImGui font atlas and invalidate the font texture
        // to re-register and re-rasterize all fonts on the next frame (see Animate)
        io.Fonts->Clear();
        io.Fonts->TexID = 0;

        for (auto& font : m_fonts)
            font->ReleaseScaledFont();

        ImGui::GetStyle() = ImGuiStyle();
        ImGui::GetStyle().ScaleAllSizes(scaleX);
    }

    void ImGuiRenderPass::BeginFullScreenWindow()
    {
        ImGuiIO const& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(
            io.DisplaySize.x / io.DisplayFramebufferScale.x,
            io.DisplaySize.y / io.DisplayFramebufferScale.y),
            ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::SetNextWindowBgAlpha(0.f);
        ImGui::Begin(" ", 0, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    }

    void ImGuiRenderPass::DrawScreenCenteredText(const char* text)
    {
        ImGuiIO const& io = ImGui::GetIO();
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImGui::SetCursorPosX((io.DisplaySize.x / io.DisplayFramebufferScale.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((io.DisplaySize.y / io.DisplayFramebufferScale.y - textSize.y) * 0.5f);
        ImGui::TextUnformatted(text);
    }

    void ImGuiRenderPass::EndFullScreenWindow()
    {
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void RegisteredFont::CreateScaledFont(float displayScale)
    {
        ImFontConfig fontConfig;
        fontConfig.SizePixels = m_sizeAtDefaultScale * displayScale;

        m_imFont = nullptr;

        if (m_data)
        {
            fontConfig.FontDataOwnedByAtlas = false;
            if (m_isCompressed)
            {
                m_imFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
                    (void*)(m_data->Data()), (int)(m_data->Size()), 0.f, &fontConfig);
            }
            else
            {
                m_imFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
                    (void*)(m_data->Data()), (int)(m_data->Size()), 0.f, &fontConfig);
            }
        }
        else if (m_isDefault)
        {
            m_imFont = ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);
        }

        if (m_imFont)
        {
            ImGui::GetIO().Fonts->TexID = 0;
        }
    }

    void RegisteredFont::ReleaseScaledFont()
    {
        m_imFont = nullptr;
    }
}