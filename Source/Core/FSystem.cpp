#include "Core/FSystem.h"

#include <format>

#include "Utility/FLog.h"

void FSystem::Initialize(int ShowCmd)
{
	Window.Initialize(Display, ShowCmd);

	FLog::Info(std::format(L"창 생성 : {}x{}", Window.GetWidth(), Window.GetHeight()));

	Window.SetMessageCallback([this](UINT Message, WPARAM WParam, LPARAM LParam)
	                          { OnWindowMessage(Message, WParam, LParam); });
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
	Application.Render();
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
