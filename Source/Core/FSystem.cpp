#include "Core/FSystem.h"

#include <format>

#include "Utility/FLog.h"

void FSystem::Initialize(int ShowCmd)
{
	Window.Initialize(Display, ShowCmd);

	const int ScalePercent = MulDiv(static_cast<int>(Window.GetDpi()), 100, USER_DEFAULT_SCREEN_DPI);
	FLog::Info(std::format(
	    L"창 생성 : {}x{} (DPI {}, 배율 {}%)", Window.GetWidth(), Window.GetHeight(), Window.GetDpi(), ScalePercent));

	Graphics.Initialize(Window, Display);

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

void FSystem::RequestExit()
{
	PostQuitMessage(0);
}

void FSystem::Frame()
{
	Application.Update();
	Graphics.BeginScene(Application.GetClearColor());
	Application.Render();
	Graphics.EndScene();
}

void FSystem::OnWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam)
{
	switch (Message)
	{
	case WM_KEYDOWN:
		Input.KeyDown(static_cast<unsigned int>(WParam));
		break;
	case WM_KEYUP:
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
