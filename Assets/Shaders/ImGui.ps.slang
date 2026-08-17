struct VSOutput
{
    float4 position : SV_Position;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

// Matches nvrhi::BindingLayoutItem::Texture_SRV(0) / Sampler(0).
Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

// c_SwapChainFormat (RHI/*/Surface.cpp, all three backends) is nvrhi::Format::SBGRA8_UNORM - an
// sRGB-typed render target. The GPU automatically linear->sRGB encodes whatever this shader
// outputs on store, and blends existing sRGB destination pixels by first decoding them to linear
// (standard fixed-function behavior for sRGB RTVs). ImGui's vertex colors (style colors, from
// ImGuiStyle) and the font atlas are authored/stored as plain sRGB-gamma bytes, not linear. Since
// this shader used to hand those gamma values straight to the sRGB RTV, the hardware treated
// already-gamma-encoded numbers as linear and re-encoded them again on store - double gamma,
// and blending performed in the wrong (gamma) space. Decoding to linear here before output fixes
// both: the store's automatic encode then reproduces the intended sRGB bytes, and the hardware's
// blend-in-linear-space becomes radiometrically correct.
//
// Note on "HDR": the swapchain is an 8-bit UNORM format on all three backends, so there is no
// output range beyond [0, 1] to preserve here - genuine HDR (values > 1.0, wide gamut) would need
// a floating-point/10-bit swapchain format wired up at the RHI::Surface level, which is out of
// reach of a shader-only fix.
float3 SrgbToLinear(float3 c)
{
    float3 cutoff = step(c, float3(0.04045, 0.04045, 0.04045));
    float3 lo = c / 12.92;
    float3 hi = pow(max((c + 0.055) / 1.055, 0.0), 2.4);
    return lerp(hi, lo, cutoff);
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target
{
    float4 texSample = fontTexture.Sample(fontSampler, input.uv);
    float3 linearColor = SrgbToLinear(input.col.rgb) * SrgbToLinear(texSample.rgb);
    float alpha = input.col.a * texSample.a;

    // Reconstructing the float4 explicitly from its own components (rather than returning a
    // texture-sample-derived value directly) works around a Slang/DXC codegen issue on the
    // D3D12/DXIL path where the output alpha channel is otherwise silently lost - see
    // tasks/todo.md Task 16 notes.
    return float4(linearColor, alpha);
}
