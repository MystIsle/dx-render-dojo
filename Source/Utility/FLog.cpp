#include "Utility/FLog.h"

#include <string>

void FLog::Info(std::wstring_view Message)
{
	Get().Write(L"", Message);
}

void FLog::Warning(std::wstring_view Message)
{
	Get().Write(L"[경고] ", Message);
}

void FLog::Error(std::wstring_view Message)
{
	Get().Write(L"[에러] ", Message);
}

FLog::FLog()
{
#ifndef NDEBUG
	if (AllocConsole())
	{
		ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTitleW(L"DxRenderDojo");
	}
#endif
}

FLog::~FLog()
{
	if (ConsoleOutput != nullptr)
	{
		FreeConsole();
	}
}

void FLog::Write(std::wstring_view Prefix, std::wstring_view Message)
{
	std::wstring Line(Prefix);
	Line += Message;
	Line += L'\n';

	OutputDebugStringW(Line.c_str());

	if (ConsoleOutput != nullptr)
	{
		DWORD Written = 0;
		WriteConsoleW(ConsoleOutput, Line.c_str(), static_cast<DWORD>(Line.size()), &Written, nullptr);
	}
}
