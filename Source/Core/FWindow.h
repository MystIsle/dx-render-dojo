#pragma once

#include <Windows.h>
#include <functional>

#include "Core/FDisplaySettings.h"

class FWindow
{
private:
	static LRESULT CALLBACK WndProc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

public:
	FWindow() = default;
	FWindow(const FWindow& Other) = delete;
	FWindow& operator=(const FWindow& Other) = delete;
	~FWindow();

	void Initialize(const FDisplaySettings& Settings, int ShowCmd);

	HWND GetHandle() const { return Handle; }
	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }

	void SetMessageCallback(std::function<void(UINT, WPARAM, LPARAM)> Callback);

private:
	LRESULT HandleMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

	HWND Handle = nullptr;
	int Width = 0;
	int Height = 0;
	bool bCursorHidden = false;
	std::function<void(UINT, WPARAM, LPARAM)> MessageCallback;
};
