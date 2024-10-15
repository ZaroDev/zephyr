struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};
struct PSOutput
{
    float4 color : SV_Target0;
};

sampler LinearSampler : register(s0);
Texture2D Texture : register(t0);

PSOutput Main(PSInput input)
{
    PSOutput output = (PSOutput) 0;
    output.color = Texture.Sample(LinearSampler, input.texCoord);;
    return output;
}