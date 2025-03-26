#pragma once


namespace Zephyr
{
	enum class FramebufferTextureFormat : u8
	{
		NONE = 0,

		RGBA8,
		RGBA16,
		RGBA32,
		RED_INTEGER,

		DEPTH24STENCIL8,

		DEPTH = DEPTH24STENCIL8
	};


	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {}


		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::NONE;
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		u32 Width = 0, Height = 0;
		FramebufferAttachmentSpecification Attachments;
		u32 Samples = 1;

		bool SwapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		Framebuffer(const FramebufferSpecification& spec) : m_Specification(spec) {}
		virtual ~Framebuffer() = default;

		DEFAULT_MOVE_AND_COPY(Framebuffer)

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(u32 width, u32 height) = 0;
		virtual i32	ReadPixel(u32 attachmentIndex, i32 x, i32 y) = 0;

		virtual void ClearAttachment(u32 attachmentIndex, Color color) = 0;

		virtual u32 GetColorAttachmentRendererID(u32 index = 0) = 0;

		const FramebufferSpecification& Specification() const { return m_Specification; }

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);

		virtual void* GetImGuiAttachment(u32 id) const = 0;

	protected:
		FramebufferSpecification m_Specification;
		std::vector<FramebufferTextureSpecification> m_ColorAttachments;
		FramebufferTextureSpecification m_DepthBuffer = FramebufferTextureFormat::DEPTH;
	};
}