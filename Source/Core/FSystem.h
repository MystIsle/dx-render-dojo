#pragma once

#include <Windows.h>

#include "App/FApplication.h"
#include "Core/FInput.h"
#include "Core/FWindow.h"
#include "Graphics/FD3D11Graphics.h"
#include "Utility/TSingleton.h"

class FSystem : public TSingleton<FSystem>
{
public:
	void Initialize(int ShowCmd);

	int Run();

private:
	void Frame();

	void OnWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam);
	void OnResize(int NewWidth, int NewHeight);

	// 멤버는 선언의 역순으로 소멸한다.
	// 창을 쓰는 멤버는 FWindow 뒤에 두어 창보다 먼저 소멸하게 한다.
	FWindow Window;
	FInput Input;
	FD3D11Graphics Graphics;
	FApplication Application;
};
