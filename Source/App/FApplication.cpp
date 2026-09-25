#include "App/FApplication.h"

#include <Windows.h>

#include "Core/FSystem.h"

void FApplication::Update()
{
	if (FSystem::Get().GetInput().IsKeyDown(VK_ESCAPE))
	{
		FSystem::Get().RequestExit();
	}
}

void FApplication::Render()
{
}
