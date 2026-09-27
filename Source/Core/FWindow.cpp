#include "Core/FWindow.h"

#include <utility>

#include "Utility/Check.h"

LRESULT CALLBACK FWindow::WndProc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
	// 치명 실패 상자가 떠 있는 동안 창 메시지가 실패한 객체에 닿지 않게 한다.
	if (::Return::IsFatal())
	{
		return DefWindowProcW(WindowHandle, Message, WParam, LParam);
	}

	if (Message == WM_NCCREATE)
	{
		const CREATESTRUCTW* CreateInfo = reinterpret_cast<const CREATESTRUCTW*>(LParam);
		SetWindowLongPtrW(WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(CreateInfo->lpCreateParams));
	}

	FWindow* Window = reinterpret_cast<FWindow*>(GetWindowLongPtrW(WindowHandle, GWLP_USERDATA));
	if (Window == nullptr)
	{
		return DefWindowProcW(WindowHandle, Message, WParam, LParam);
	}

	return Window->HandleMessage(WindowHandle, Message, WParam, LParam);
}

FWindow::~FWindow()
{
	// 콜백을 먼저 끊는다. DestroyWindow 가 부르는 메시지가 이미 사라진 FInput·FApplication 에 닿지 않게 한다.
	MessageCallback = nullptr;

	if (bCursorHidden)
	{
		ShowCursor(TRUE);
	}

	if (Handle != nullptr)
	{
		DestroyWindow(Handle);
	}
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

void FWindow::SetMessageCallback(std::function<void(UINT, WPARAM, LPARAM)> Callback)
{
	MessageCallback = std::move(Callback);
}

LRESULT FWindow::HandleMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
	switch (Message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	// 윈도우가 꺼질 때는 WM_CLOSE 가 오지 않는다. 종료 신호를 같은 길로 모은다.
	// WParam 이 FALSE 면 다른 앱이 종료를 거부한 것이라 그대로 둔다.
	case WM_ENDSESSION:
		if (WParam == TRUE)
		{
			PostQuitMessage(0);
		}
		return 0;

	default:
		if (MessageCallback)
		{
			MessageCallback(Message, WParam, LParam);
		}
		return DefWindowProcW(WindowHandle, Message, WParam, LParam);
	}
}
