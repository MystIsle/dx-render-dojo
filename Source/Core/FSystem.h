#pragma once

#include <Windows.h>

#include "App/FApplication.h"
#include "Core/FDisplaySettings.h"
#include "Core/FInput.h"
#include "Core/FWindow.h"
#include "Utility/TSingleton.h"

class FSystem : public TSingleton<FSystem>
{
public:
	void Initialize(int ShowCmd);

	const FInput& GetInput() const { return Input; }

	int Run();
	void RequestExit();

private:
	void Frame();

	void OnWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam);

	// 선언 순서가 곧 생성 순서이고 소멸은 그 역순이다.
	// 창을 쓰는 멤버는 FWindow 뒤에 둔다.
	FDisplaySettings Display;
	FWindow Window;
	FInput Input;
	FApplication Application;
};
