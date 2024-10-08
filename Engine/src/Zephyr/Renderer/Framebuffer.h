#pragma once


namespace Zephyr
{
	enum class FramebufferTextureFormat
	{
		NONE = 0,

		RGBA8,
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
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(u32 width, u32 height) = 0;
		virtual i32	ReadPixel(u32 attachmentIndex, i32 x, i32 y) = 0;

		virtual void ClearAttachment(u32 attachmentIndex, i32 value) = 0;

		virtual u32 GetColorAttachmentRendererID(u32 index = 0) = 0;

		virtual const FramebufferSpecification& Specification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};
}