#include "pch.h"
#include "ImGuiRenderer.h"

#include <ImGui/imgui_impl_glfw.h>

namespace Zephyr
{
	// ImTextureID is a 64-bit integer in this ImGui version (not a raw void* like older
	// versions) - round-trip nvrhi::ITexture* through it via intptr_t.
	static ImTextureID ToImTextureID(nvrhi::ITexture* texture)
	{
		return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture));
	}
	static nvrhi::ITexture* FromImTextureID(ImTextureID id)
	{
		return reinterpret_cast<nvrhi::ITexture*>(static_cast<intptr_t>(id));
	}

	ImGuiRenderer::ImGuiRenderer(const Ref<Device>& device, ShaderFactory& shaderFactory, GLFWwindow* windowHandle)
		: m_Device(device)
	{
		ImGui::CreateContext();

		// InitForOther (not InitForVulkan/InitForOpenGL): this class renders via nvrhi/RHI::Device
		// itself, not the vendored imgui_impl_vulkan/opengl3 backends the Init*For* variants
		// otherwise imply - install_callbacks=true registers GLFW's key/char/mouse/scroll/focus
		// callbacks (chaining any pre-existing ones, e.g. Window's WindowCloseCallback, which use a
		// different callback slot) and feeds them into ImGuiIO for us.
		ImGui_ImplGlfw_InitForOther(windowHandle, true);

		m_DefaultFont = CreateRef<RegisteredFont>(13.f);
		m_Fonts.push_back(m_DefaultFont);

		m_CommandList = CreateScope<CommandList>(m_Device->GetNvrhiDevice());

		m_VertexShader = shaderFactory.CreateShader("ImGui.vs.slang", "main", nullptr, nvrhi::ShaderType::Vertex);
		m_PixelShader = shaderFactory.CreateShader("ImGui.ps.slang", "main", nullptr, nvrhi::ShaderType::Pixel);
		CORE_ASSERT(m_VertexShader && m_PixelShader, "Failed to create the ImGui shaders");

		nvrhi::IDevice* nvrhiDevice = m_Device->GetNvrhiDevice();

		nvrhi::VertexAttributeDesc vertexAttribLayout[] = {
			{ "POSITION", nvrhi::Format::RG32_FLOAT,  1, 0, offsetof(ImDrawVert, pos), sizeof(ImDrawVert), false },
			{ "TEXCOORD", nvrhi::Format::RG32_FLOAT,  1, 0, offsetof(ImDrawVert, uv),  sizeof(ImDrawVert), false },
			{ "COLOR",    nvrhi::Format::RGBA8_UNORM, 1, 0, offsetof(ImDrawVert, col), sizeof(ImDrawVert), false },
		};
		m_ShaderAttribLayout = nvrhiDevice->createInputLayout(vertexAttribLayout, u32(std::size(vertexAttribLayout)), m_VertexShader);

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
		m_BindingLayout = nvrhiDevice->createBindingLayout(layoutDesc);

		m_BasePSODesc.primType = nvrhi::PrimitiveType::TriangleList;
		m_BasePSODesc.inputLayout = m_ShaderAttribLayout;
		m_BasePSODesc.VS = m_VertexShader;
		m_BasePSODesc.PS = m_PixelShader;
		m_BasePSODesc.renderState = renderState;
		m_BasePSODesc.bindingLayouts = { m_BindingLayout };

		const auto samplerDesc = nvrhi::SamplerDesc()
			.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
			.setAllFilters(true);
		m_FontSampler = nvrhiDevice->createSampler(samplerDesc);
		CORE_ASSERT(m_FontSampler, "Failed to create the ImGui font sampler");
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		// Must run before DestroyContext() (ImGui_ImplGlfw_Shutdown() touches ImGui::GetIO()/
		// GetMainViewport()) and while the GLFW window is still valid (ModuleManager::Shutdown()
		// runs Renderer::Shutdown(), which destroys this, before Window::Shutdown() - see the
		// priority-reverse-order fix in ModuleManager::Shutdown()).
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	Ref<RegisteredFont> ImGuiRenderer::CreateFontFromFile(IFileSystem& fs, const Path& fontFile, float fontSize)
	{
		auto fontData = fs.ReadFile(fontFile);
		if (!fontData)
			return CreateRef<RegisteredFont>();

		auto font = CreateRef<RegisteredFont>(fontData, false, fontSize);
		m_Fonts.push_back(font);
		return font;
	}

	Ref<RegisteredFont> ImGuiRenderer::CreateFontFromMemoryInternal(void const* pData, size_t size, bool compressed, float fontSize)
	{
		if (!pData || !size)
			return CreateRef<RegisteredFont>();

		void* dataCopy = malloc(size);
		memcpy(dataCopy, pData, size);
		Ref<Blob> blob = CreateRef<Blob>(dataCopy, size);

		auto font = CreateRef<RegisteredFont>(blob, compressed, fontSize);
		m_Fonts.push_back(font);
		return font;
	}

	Ref<RegisteredFont> ImGuiRenderer::CreateFontFromMemory(void const* pData, size_t size, float fontSize)
	{
		return CreateFontFromMemoryInternal(pData, size, false, fontSize);
	}

	Ref<RegisteredFont> ImGuiRenderer::CreateFontFromMemoryCompressed(void const* pData, size_t size, float fontSize)
	{
		return CreateFontFromMemoryInternal(pData, size, true, fontSize);
	}

	bool ImGuiRenderer::UpdateFontTexture()
	{
		ImGuiIO& io = ImGui::GetIO();

		if (m_FontTexture && io.Fonts->TexID)
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

		m_FontTexture = m_Device->GetNvrhiDevice()->createTexture(textureDesc);
		if (!m_FontTexture)
			return false;

		nvrhi::ICommandList* commandList = m_CommandList->GetNvrhiCommandList();
		m_CommandList->Open();
		commandList->beginTrackingTextureState(m_FontTexture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
		commandList->writeTexture(m_FontTexture, 0, 0, pixels, width * 4);
		commandList->setPermanentTextureState(m_FontTexture, nvrhi::ResourceStates::ShaderResource);
		commandList->commitBarriers();
		m_CommandList->Close();
		m_Device->GetNvrhiDevice()->executeCommandList(commandList);

		io.Fonts->SetTexID(ToImTextureID(m_FontTexture));

		return true;
	}

	bool ImGuiRenderer::ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer)
	{
		if (buffer == nullptr || size_t(buffer->getDesc().byteSize) < requiredSize)
		{
			nvrhi::BufferDesc desc;
			desc.byteSize = uint32_t(reallocateSize);
			desc.structStride = 0;
			desc.debugName = isIndexBuffer ? "ImGui index buffer" : "ImGui vertex buffer";
			desc.canHaveUAVs = false;
			desc.isVertexBuffer = !isIndexBuffer;
			desc.isIndexBuffer = isIndexBuffer;
			desc.isDrawIndirectArgs = false;
			desc.isVolatile = false;
			desc.initialState = isIndexBuffer ? nvrhi::ResourceStates::IndexBuffer : nvrhi::ResourceStates::VertexBuffer;
			desc.keepInitialState = true;

			buffer = m_Device->GetNvrhiDevice()->createBuffer(desc);
			if (!buffer)
				return false;
		}

		return true;
	}

	nvrhi::IGraphicsPipeline* ImGuiRenderer::GetPSO(nvrhi::IFramebuffer* framebuffer)
	{
		if (m_Pso)
			return m_Pso;

		m_Pso = m_Device->GetNvrhiDevice()->createGraphicsPipeline(m_BasePSODesc, framebuffer);
		CORE_ASSERT(m_Pso, "Failed to create the ImGui graphics pipeline");
		return m_Pso;
	}

	nvrhi::IBindingSet* ImGuiRenderer::GetBindingSet(nvrhi::ITexture* texture)
	{
		auto it = m_BindingsCache.find(texture);
		if (it != m_BindingsCache.end())
			return it->second;

		nvrhi::BindingSetDesc desc;
		desc.bindings = {
			nvrhi::BindingSetItem::PushConstants(0, sizeof(float) * 2),
			nvrhi::BindingSetItem::Texture_SRV(0, texture),
			nvrhi::BindingSetItem::Sampler(0, m_FontSampler)
		};

		nvrhi::BindingSetHandle binding = m_Device->GetNvrhiDevice()->createBindingSet(desc, m_BindingLayout);
		CORE_ASSERT(binding, "Failed to create an ImGui binding set");

		m_BindingsCache[texture] = binding;
		return binding;
	}

	bool ImGuiRenderer::UpdateGeometry(nvrhi::ICommandList* commandList)
	{
		ImDrawData* drawData = ImGui::GetDrawData();

		if (!ReallocateBuffer(m_VertexBuffer,
			drawData->TotalVtxCount * sizeof(ImDrawVert),
			(drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert),
			false))
		{
			return false;
		}

		if (!ReallocateBuffer(m_IndexBuffer,
			drawData->TotalIdxCount * sizeof(ImDrawIdx),
			(drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx),
			true))
		{
			return false;
		}

		m_VtxScratch.resize(m_VertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
		m_IdxScratch.resize(m_IndexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

		ImDrawVert* vtxDst = m_VtxScratch.data();
		ImDrawIdx* idxDst = m_IdxScratch.data();
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];

			memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

			vtxDst += cmdList->VtxBuffer.Size;
			idxDst += cmdList->IdxBuffer.Size;
		}

		commandList->writeBuffer(m_VertexBuffer, m_VtxScratch.data(), m_VertexBuffer->getDesc().byteSize);
		commandList->writeBuffer(m_IndexBuffer, m_IdxScratch.data(), m_IndexBuffer->getDesc().byteSize);

		return true;
	}

	void ImGuiRenderer::Animate(float elapsedTimeSeconds, u32 windowWidth, u32 windowHeight, float scaleX, float scaleY)
	{
		for (auto& font : m_Fonts)
		{
			if (!font->GetScaledFont())
				font->CreateScaledFont(scaleX);
		}

		UpdateFontTexture();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(float(windowWidth), float(windowHeight));
		io.DisplayFramebufferScale = ImVec2(scaleX, scaleY);
		io.DeltaTime = elapsedTimeSeconds > 0.f ? elapsedTimeSeconds : 1.f / 60.f;
		io.MouseDrawCursor = false;

		ImGui::NewFrame();
	}

	void ImGuiRenderer::Render(nvrhi::IFramebuffer* framebuffer)
	{
		ImGui::Render();

		ImDrawData* drawData = ImGui::GetDrawData();
		const ImGuiIO& io = ImGui::GetIO();

		nvrhi::ICommandList* commandList = m_CommandList->GetNvrhiCommandList();
		m_CommandList->Open();
		commandList->beginMarker("ImGui");

		if (!UpdateGeometry(commandList))
		{
			commandList->endMarker();
			m_CommandList->Close();
			m_Device->GetNvrhiDevice()->executeCommandList(commandList);
			return;
		}

		drawData->ScaleClipRects(io.DisplayFramebufferScale);

		const float invDisplaySize[2] = { 1.f / io.DisplaySize.x, 1.f / io.DisplaySize.y };

		nvrhi::GraphicsState drawState;
		drawState.framebuffer = framebuffer;
		drawState.pipeline = GetPSO(framebuffer);
		drawState.viewport.addViewport(nvrhi::Viewport(
			io.DisplaySize.x * io.DisplayFramebufferScale.x,
			io.DisplaySize.y * io.DisplayFramebufferScale.y));
		drawState.viewport.scissorRects.resize(1);

		nvrhi::VertexBufferBinding vbufBinding;
		vbufBinding.buffer = m_VertexBuffer;
		vbufBinding.slot = 0;
		vbufBinding.offset = 0;
		drawState.vertexBuffers.push_back(vbufBinding);

		drawState.indexBuffer.buffer = m_IndexBuffer;
		drawState.indexBuffer.format = sizeof(ImDrawIdx) == 2 ? nvrhi::Format::R16_UINT : nvrhi::Format::R32_UINT;
		drawState.indexBuffer.offset = 0;

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
				else
				{
					drawState.bindings = { GetBindingSet(FromImTextureID(pCmd->TextureId)) };

					drawState.viewport.scissorRects[0] = nvrhi::Rect(
						int(pCmd->ClipRect.x), int(pCmd->ClipRect.z),
						int(pCmd->ClipRect.y), int(pCmd->ClipRect.w));

					nvrhi::DrawArguments drawArguments;
					drawArguments.vertexCount = pCmd->ElemCount;
					drawArguments.startIndexLocation = idxOffset;
					drawArguments.startVertexLocation = vtxOffset;

					commandList->setGraphicsState(drawState);
					commandList->setPushConstants(invDisplaySize, sizeof(invDisplaySize));
					commandList->drawIndexed(drawArguments);
				}

				idxOffset += pCmd->ElemCount;
			}

			vtxOffset += cmdList->VtxBuffer.Size;
		}

		commandList->endMarker();
		m_CommandList->Close();
		m_Device->GetNvrhiDevice()->executeCommandList(commandList);
	}

	void ImGuiRenderer::BackBufferResizing()
	{
		m_Pso = nullptr;
	}

	void ImGuiRenderer::DisplayScaleChanged(float scaleX, float scaleY)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Clear the font atlas and invalidate the font texture so all fonts are re-rasterized
		// at the new scale on the next Animate()/UpdateFontTexture() call.
		io.Fonts->Clear();
		io.Fonts->TexID = 0;
		m_FontTexture = nullptr;

		for (auto& font : m_Fonts)
			font->ReleaseScaledFont();

		ImGui::GetStyle() = ImGuiStyle();
		ImGui::GetStyle().ScaleAllSizes(scaleX);
	}

	void RegisteredFont::CreateScaledFont(float displayScale)
	{
		ImFontConfig fontConfig;
		fontConfig.SizePixels = m_SizeAtDefaultScale * displayScale;

		m_ImFont = nullptr;

		if (m_Data)
		{
			fontConfig.FontDataOwnedByAtlas = false;
			if (m_IsCompressed)
			{
				m_ImFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
					(void*)(m_Data->Data()), (int)(m_Data->Size()), 0.f, &fontConfig);
			}
			else
			{
				m_ImFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
					(void*)(m_Data->Data()), (int)(m_Data->Size()), 0.f, &fontConfig);
			}
		}
		else if (m_IsDefault)
		{
			m_ImFont = ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);
		}

		if (m_ImFont)
		{
			ImGui::GetIO().Fonts->TexID = 0;
		}
	}

	void RegisteredFont::ReleaseScaledFont()
	{
		m_ImFont = nullptr;
	}
}
