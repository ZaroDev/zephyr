struct PSInput
{
    float4 position : SV_Position;
    float3 fragPos : COLOR0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};
struct PSOutput
{
    float4 color : SV_Target0;
};

static const float3 c_LightPos = float3(1.0f, 1.0f, 1.0f);

PSOutput Main(PSInput input)
{
    PSOutput output = (PSOutput) 0;
    float3 norm = normalize(input.normal);
    float3 lightDir = normalize(c_LightPos - input.fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
   
    output.color = diff;
    return output;
}