struct FVertexInput
{
	float3 Position : POSITION;
	float4 Color : COLOR;
};

// 정점 셰이더의 출력이 픽셀 셰이더의 입력이 된다. 두 파일의 FPixelInput 이 같아야 한다.
struct FPixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

FPixelInput ColorVertexShader(FVertexInput Input)
{
	FPixelInput Output;
	Output.Position = float4(Input.Position, 1.0f);
	Output.Color = Input.Color;
	return Output;
}
