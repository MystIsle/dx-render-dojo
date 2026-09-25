#include "Core/FSystem.h"

#include <format>

#include "Utility/FLog.h"

void FSystem::Initialize(int ShowCmd)
{
	Window.Initialize(Display, ShowCmd);

	FLog::Info(std::format(L"창 생성 : {}x{}", Window.GetWidth(), Window.GetHeight()));
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
	}

	return static_cast<int>(Message.wParam);
}
