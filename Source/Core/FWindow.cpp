#include "Core/FWindow.h"

#include "Utility/Check.h"

LRESULT CALLBACK FWindow::WndProc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
	return DefWindowProcW(WindowHandle, Message, WParam, LParam);
}

void FWindow::Initialize(const FDisplaySettings& Settings, int ShowCmd)
{
	constexpr const wchar_t* ClassName = L"DxRenderDojo";

	const HINSTANCE Instance = GetModuleHandleW(nullptr);

	WNDCLASSEXW WindowClass = {};
	WindowClass.cbSize = sizeof(WindowClass);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = &FWindow::WndProc;
	WindowClass.hInstance = Instance;
	WindowClass.hIcon = LoadIconW(nullptr, IDI_WINLOGO);
	WindowClass.hIconSm = WindowClass.hIcon;
	WindowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	WindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	WindowClass.lpszClassName = ClassName;
	CHECK_FATAL(RegisterClassExW(&WindowClass) != 0);

	const int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	DWORD Style = WS_POPUP;
	int PositionX = 0;
	int PositionY = 0;
	int OuterWidth = ScreenWidth;
	int OuterHeight = ScreenHeight;

	if (Settings.bFullScreen == false)
	{
		Style = WS_OVERLAPPEDWINDOW;

		RECT Bounds = {0, 0, Settings.Width, Settings.Height};
		AdjustWindowRect(&Bounds, Style, FALSE);
		OuterWidth = Bounds.right - Bounds.left;
		OuterHeight = Bounds.bottom - Bounds.top;
		PositionX = (ScreenWidth - OuterWidth) / 2;
		PositionY = (ScreenHeight - OuterHeight) / 2;
	}

	Handle = CreateWindowExW(0,
	                         ClassName,
	                         ClassName,
	                         Style,
	                         PositionX,
	                         PositionY,
	                         OuterWidth,
	                         OuterHeight,
	                         nullptr,
	                         nullptr,
	                         Instance,
	                         this);
	CHECK_FATAL(Handle);

	RECT Client = {};
	GetClientRect(Handle, &Client);
	Width = Client.right - Client.left;
	Height = Client.bottom - Client.top;

	if (Settings.bHideCursor)
	{
		ShowCursor(FALSE);
		bCursorHidden = true;
	}

	ShowWindow(Handle, ShowCmd);
}
