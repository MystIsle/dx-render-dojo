#include "Core/FWindow.h"

#include <format>
#include <utility>

#include "Utility/Check.h"
#include "Utility/FLog.h"

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
	ResizeCallback = nullptr;

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

	constexpr DWORD Style = WS_OVERLAPPEDWINDOW;

	// DPI 는 창이 생긴 뒤에야 알 수 있다. 주 모니터 원점에 만들어 DPI 를 읽고 크기와 위치를 다시 맞춘다.
	Handle = CreateWindowExW(
	    0, ClassName, ClassName, Style, 0, 0, Settings.Width, Settings.Height, nullptr, nullptr, Instance, this);
	CHECK_FATAL(Handle);

	Dpi = GetDpiForWindow(Handle);

	RECT Bounds = {0, 0, ScaleForDpi(Settings.Width), ScaleForDpi(Settings.Height)};
	AdjustWindowRectExForDpi(&Bounds, Style, FALSE, 0, Dpi);
	const int OuterWidth = Bounds.right - Bounds.left;
	const int OuterHeight = Bounds.bottom - Bounds.top;
	const int PositionX = (GetSystemMetrics(SM_CXSCREEN) - OuterWidth) / 2;
	const int PositionY = (GetSystemMetrics(SM_CYSCREEN) - OuterHeight) / 2;
	SetWindowPos(Handle, nullptr, PositionX, PositionY, OuterWidth, OuterHeight, SWP_NOZORDER | SWP_NOACTIVATE);

	if (Settings.bFullScreen)
	{
		ToggleFullScreen();
	}

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

void FWindow::SetResizeCallback(std::function<void(int, int)> Callback)
{
	ResizeCallback = std::move(Callback);
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

	// 드래그하는 동안에는 알리지 않고 끝날 때 한 번만 알린다. 매 픽셀마다 스왑체인을 다시 만들지 않기 위해서다.
	case WM_ENTERSIZEMOVE:
		bSizing = true;
		return 0;

	case WM_EXITSIZEMOVE:
		bSizing = false;
		NotifyResize();
		return 0;

	case WM_SIZE:
		if (WParam != SIZE_MINIMIZED && bSizing == false)
		{
			NotifyResize();
		}
		return 0;

	// 배율이 바뀌면 윈도우는 바깥 크기를 비율대로 키운다. 테두리는 비율대로 커지지 않아 안쪽 크기가 어긋나므로,
	// 안쪽 크기를 기준으로 계산한 바깥 크기를 알려 준다. 전체 화면과 최대화는 윈도우에 맡긴다.
	case WM_GETDPISCALEDSIZE:
	{
		if (bFullScreen || IsZoomed(WindowHandle))
		{
			return FALSE;
		}

		const int NewDpi = static_cast<int>(WParam);
		RECT Client = {};
		GetClientRect(WindowHandle, &Client);

		RECT Bounds = {0,
		               0,
		               MulDiv(Client.right, NewDpi, static_cast<int>(Dpi)),
		               MulDiv(Client.bottom, NewDpi, static_cast<int>(Dpi))};
		const DWORD Style = static_cast<DWORD>(GetWindowLongPtrW(WindowHandle, GWL_STYLE));
		const DWORD ExStyle = static_cast<DWORD>(GetWindowLongPtrW(WindowHandle, GWL_EXSTYLE));
		AdjustWindowRectExForDpi(&Bounds, Style, FALSE, ExStyle, static_cast<UINT>(NewDpi));

		SIZE* Size = reinterpret_cast<SIZE*>(LParam);
		Size->cx = Bounds.right - Bounds.left;
		Size->cy = Bounds.bottom - Bounds.top;
		return TRUE;
	}

	// 배율이 다른 모니터로 옮기거나 배율 설정을 바꾸면 온다.
	// 창 모드에서는 윈도우가 제안한 사각형을 그대로 쓰고, 전체 화면에서는 모니터를 다시 채운다.
	case WM_DPICHANGED:
	{
		Dpi = HIWORD(WParam);
		FLog::Info(std::format(L"DPI 변경 : {} (배율 {}%)", Dpi, ScaleForDpi(100)));

		if (bFullScreen)
		{
			FitToMonitor();
			return 0;
		}

		const RECT* Suggested = reinterpret_cast<const RECT*>(LParam);
		SetWindowPos(WindowHandle,
		             nullptr,
		             Suggested->left,
		             Suggested->top,
		             Suggested->right - Suggested->left,
		             Suggested->bottom - Suggested->top,
		             SWP_NOZORDER | SWP_NOACTIVATE);
		return 0;
	}

	case WM_GETMINMAXINFO:
	{
		constexpr int MinimumWidth = 320;
		constexpr int MinimumHeight = 240;

		MINMAXINFO* Limits = reinterpret_cast<MINMAXINFO*>(LParam);
		Limits->ptMinTrackSize.x = ScaleForDpi(MinimumWidth);
		Limits->ptMinTrackSize.y = ScaleForDpi(MinimumHeight);
		return 0;
	}

	// Alt 와 함께 누른 키에 맞는 메뉴가 없으면 윈도우가 경고음을 낸다. Alt+Enter 마다 소리가 나지 않게 한다.
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	// Alt+Enter 로 전체 화면을 켜고 끈다. 누르고 있는 동안 반복해서 오는 메시지는 건너뛴다.
	// 그 밖의 키는 default 로 넘겨 Alt+F4 같은 시스템 동작이 그대로 돈다.
	case WM_SYSKEYDOWN:
		if (WParam == VK_RETURN && (HIWORD(LParam) & KF_ALTDOWN) != 0 && (HIWORD(LParam) & KF_REPEAT) == 0)
		{
			ToggleFullScreen();
			return 0;
		}
		[[fallthrough]];

	default:
		if (MessageCallback)
		{
			MessageCallback(Message, WParam, LParam);
		}
		return DefWindowProcW(WindowHandle, Message, WParam, LParam);
	}
}

void FWindow::NotifyResize()
{
	RECT Client = {};
	GetClientRect(Handle, &Client);
	const int NewWidth = Client.right - Client.left;
	const int NewHeight = Client.bottom - Client.top;

	// 창을 옮기기만 해도 WM_EXITSIZEMOVE 가 온다. 크기가 그대로면 스왑체인을 다시 만들 이유가 없다.
	if (NewWidth == Width && NewHeight == Height)
	{
		return;
	}

	Width = NewWidth;
	Height = NewHeight;

	if (ResizeCallback)
	{
		ResizeCallback(Width, Height);
	}
}

int FWindow::ScaleForDpi(int Value) const
{
	return MulDiv(Value, static_cast<int>(Dpi), USER_DEFAULT_SCREEN_DPI);
}

// 테두리 없는 전체 화면. 제목 표시줄과 테두리를 빼고 창이 있는 모니터를 채운다.
// 되돌릴 때 쓰려고 원래 위치·크기·최대화 상태를 저장해 둔다.
// 전환하는 동안 크기가 여러 번 바뀌므로 알림은 끝난 뒤 한 번만 보낸다.
void FWindow::ToggleFullScreen()
{
	bFullScreen = (bFullScreen == false);
	FLog::Info(bFullScreen ? L"전체 화면 : 켬" : L"전체 화면 : 끔");

	bSizing = true;

	const LONG_PTR Style = GetWindowLongPtrW(Handle, GWL_STYLE);
	if (bFullScreen)
	{
		SavedPlacement.length = sizeof(SavedPlacement);
		GetWindowPlacement(Handle, &SavedPlacement);
		SetWindowLongPtrW(Handle, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
		FitToMonitor();
	}
	else
	{
		SetWindowLongPtrW(Handle, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Handle, &SavedPlacement);
		SetWindowPos(
		    Handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}

	bSizing = false;
	NotifyResize();
}

void FWindow::FitToMonitor()
{
	MONITORINFO Monitor = {};
	Monitor.cbSize = sizeof(Monitor);
	GetMonitorInfoW(MonitorFromWindow(Handle, MONITOR_DEFAULTTONEAREST), &Monitor);

	const RECT& Bounds = Monitor.rcMonitor;
	SetWindowPos(Handle,
	             HWND_TOP,
	             Bounds.left,
	             Bounds.top,
	             Bounds.right - Bounds.left,
	             Bounds.bottom - Bounds.top,
	             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}
