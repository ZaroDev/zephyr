struct VSOutput
{
    float4 position : SV_Position;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

// Matches nvrhi::BindingLayoutItem::Texture_SRV(0) / Sampler(0).
Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

[shader("fragment")]
float4 main(VSOutput input) : SV_Target
{
    float4 c = input.col * fontTexture.Sample(fontSampler, input.uv);
    // Returning `c` directly (or the multiply expression inline) compiles fine but the D3D12/DXIL
    // output's alpha channel is effectively lost - every pixel blends as if alpha were 0, even
    // though c.a reads correctly in isolation (verified by branching on it). Reconstructing the
    // float4 explicitly from its own components avoids whatever Slang/DXC codegen path causes
    // that - empirically confirmed fix, root cause not identified further.
    return float4(c.rgb, c.a);
}
