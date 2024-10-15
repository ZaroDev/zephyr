struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 fragPos : COLOR0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

cbuffer Scene : register(b0)
{
    matrix Model;
    matrix View;
    matrix Projection;
};

VSOutput Main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    
    matrix VP = mul(Projection, View);
    output.fragPos = mul(Model, float4(input.position, 1.0)).xyz;
    output.position = mul(VP, float4(output.fragPos, 1.0));
    output.normal = input.normal;
    output.texCoord = input.texCoord;
    
    return output;
}