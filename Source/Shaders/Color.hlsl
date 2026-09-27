cbuffer MatrixBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
};

struct FVertexInput
{
	float3 Position : POSITION;
	float4 Color : COLOR;
};

// 정점 셰이더의 출력이 픽셀 셰이더의 입력이 된다.
struct FPixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

FPixelInput ColorVertexShader(FVertexInput Input)
{
	FPixelInput Output;
	Output.Position = mul(float4(Input.Position, 1.0f), World);
	Output.Position = mul(Output.Position, View);
	Output.Position = mul(Output.Position, Projection);
	Output.Color = Input.Color;
	return Output;
}

float4 ColorPixelShader(FPixelInput Input) : SV_TARGET
{
	return Input.Color;
}
