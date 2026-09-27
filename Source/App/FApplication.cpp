#include "App/FApplication.h"

#include "Graphics/FD3D11Graphics.h"

void FApplication::Initialize(FD3D11Graphics& Graphics)
{
	Model.Initialize(Graphics.GetDevice());
	ColorShader.Initialize(Graphics.GetDevice());
}

void FApplication::Update()
{
}

void FApplication::Render(FD3D11Graphics& Graphics)
{
	ID3D11DeviceContext* Context = Graphics.GetContext();
	ColorShader.Bind(Context);
	Model.Draw(Context);
}
