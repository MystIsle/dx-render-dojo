#include "App/FApplication.h"

#include "Graphics/FD3D11Graphics.h"

using namespace DirectX;

void FApplication::Initialize(FD3D11Graphics& Graphics)
{
	Camera.SetPosition(0.0f, 0.0f, -5.0f);
	Model.Initialize(Graphics.GetDevice());
	ColorShader.Initialize(Graphics.GetDevice());
}

void FApplication::Update()
{
}

void FApplication::Render(FD3D11Graphics& Graphics)
{
	const XMMATRIX World = XMMatrixIdentity();
	const XMMATRIX View = Camera.GetViewMatrix();
	const XMMATRIX Projection = Camera.GetProjectionMatrix(Graphics.GetAspectRatio());

	ID3D11DeviceContext* Context = Graphics.GetContext();
	ColorShader.SetMatrices(Context, World, View, Projection);
	ColorShader.Bind(Context);
	Model.Draw(Context);
}
