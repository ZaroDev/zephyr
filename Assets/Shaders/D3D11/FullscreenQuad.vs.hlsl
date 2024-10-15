struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VSOutput Main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    output.position = float4(input.position, 1.0);
    output.texCoord = input.texCoord;
    
    return output;
}