struct VSInput
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD;
    float4 col : COLOR;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

// Matches nvrhi::BindingLayoutItem::PushConstants(0, sizeof(float) * 2) - register(b0) is used
// on D3D targets, [[vk::push_constant]] makes Slang emit a real push-constant block for Vulkan.
[[vk::push_constant]]
cbuffer PushConstants : register(b0)
{
    float2 invDisplaySize;
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.pos * invDisplaySize * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    output.col = input.col;
    output.uv = input.uv;
    return output;
}
