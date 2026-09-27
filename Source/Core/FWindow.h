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
	void SetResizeCallback(std::function<void(int, int)> Callback);

private:
	LRESULT HandleMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);
	void NotifyResize();

	HWND Handle = nullptr;
	int Width = 0;
	int Height = 0;
	bool bCursorHidden = false;
	bool bSizing = false;
	std::function<void(UINT, WPARAM, LPARAM)> MessageCallback;
	std::function<void(int, int)> ResizeCallback;
};
