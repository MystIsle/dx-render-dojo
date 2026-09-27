#include "Core/FSystem.h"

#include <format>

#include "Core/FDisplaySettings.h"
#include "Utility/FLog.h"

void FSystem::Initialize(int ShowCmd)
{
	const FDisplaySettings Settings;
	Window.Initialize(Settings, ShowCmd);

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

void FSystem::Frame()
{
	if (Input.IsKeyDown(VK_ESCAPE))
	{
		PostQuitMessage(0);
	}

	Application.Update();
	Application.Render();
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
