#include "Core/FSystem.h"

#include <format>

#include "Core/FDisplaySettings.h"
#include "Utility/FLog.h"

void FSystem::Initialize(int ShowCmd)
{
	const FDisplaySettings Settings;
	Window.Initialize(Settings, ShowCmd);

	const int ScalePercent = MulDiv(static_cast<int>(Window.GetDpi()), 100, USER_DEFAULT_SCREEN_DPI);
	FLog::Info(std::format(
	    L"창 생성 : {}x{} (DPI {}, 배율 {}%)", Window.GetWidth(), Window.GetHeight(), Window.GetDpi(), ScalePercent));

	Graphics.Initialize(Window.GetHandle(), Window.GetWidth(), Window.GetHeight(), Settings.bVSync);
	Application.Initialize(Graphics);

	Window.SetMessageCallback([this](UINT Message, WPARAM WParam, LPARAM LParam)
	                          { OnWindowMessage(Message, WParam, LParam); });
	Window.SetResizeCallback([this](int NewWidth, int NewHeight) { OnResize(NewWidth, NewHeight); });
}

int FSystem::Run()
{
	MSG Message = {};
	while (Message.message != WM_QUIT)
	{
		if (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessageW(&Message);
			continue;
		}

		Frame();
	}

	return static_cast<int>(Message.wParam);
}

void FSystem::Frame()
{
	if (Input.IsKeyDown(VK_ESCAPE))
	{
		PostQuitMessage(0);
	}

	Application.Update();
	Graphics.BeginScene();
	Application.Render();
	Graphics.EndScene();
}

void FSystem::OnWindowMessage(UINT Message, WPARAM WParam, [[maybe_unused]] LPARAM LParam)
{
	switch (Message)
	{
	// Alt 를 누른 채 누른 키와 뗀 키는 WM_SYSKEYDOWN·WM_SYSKEYUP 으로 온다.
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		Input.KeyDown(static_cast<unsigned int>(WParam));
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Input.KeyUp(static_cast<unsigned int>(WParam));
		break;
	default:
		break;
	}
}

void FSystem::OnResize(int NewWidth, int NewHeight)
{
	FLog::Info(std::format(L"창 크기 변경 : {}x{}", NewWidth, NewHeight));
	Graphics.Resize(NewWidth, NewHeight);
}
