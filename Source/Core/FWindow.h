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
	UINT GetDpi() const { return Dpi; }

	void SetMessageCallback(std::function<void(UINT, WPARAM, LPARAM)> Callback);
	void SetResizeCallback(std::function<void(int, int)> Callback);

private:
	LRESULT HandleMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);
	void NotifyResize();
	int ScaleForDpi(int Value) const;
	void ToggleFullScreen();
	void FitToMonitor();

	HWND Handle = nullptr;
	int Width = 0;
	int Height = 0;
	UINT Dpi = USER_DEFAULT_SCREEN_DPI;
	bool bCursorHidden = false;
	bool bSizing = false;
	bool bFullScreen = false;
	WINDOWPLACEMENT SavedPlacement = {};
	std::function<void(UINT, WPARAM, LPARAM)> MessageCallback;
	std::function<void(int, int)> ResizeCallback;
};
