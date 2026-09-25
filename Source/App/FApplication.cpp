#include "App/FApplication.h"

#include <Windows.h>

#include "Core/FSystem.h"
#include "Graphics/FD3D11Graphics.h"

using namespace DirectX;

void FApplication::Initialize(FD3D11Graphics& Graphics)
{
	Model.Initialize(Graphics.GetDevice());
	ColorShader.Initialize(Graphics.GetDevice());
}

void FApplication::Update()
{
	if (FSystem::Get().GetInput().IsKeyDown(VK_ESCAPE))
	{
		FSystem::Get().RequestExit();
	}
}

void FApplication::Render(FD3D11Graphics& Graphics)
{
	const XMMATRIX World = XMMatrixIdentity();
	const XMMATRIX View = XMMatrixIdentity();
	const XMMATRIX Projection = XMMatrixIdentity();

	ID3D11DeviceContext* Context = Graphics.GetContext();
	Model.Render(Context);
	ColorShader.Render(Context, Model.GetIndexCount(), World, View, Projection);
}
