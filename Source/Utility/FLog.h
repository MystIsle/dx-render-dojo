#pragma once

#include <Windows.h>
#include <string_view>

#include "Utility/TSingleton.h"

class FLog : public TSingleton<FLog>
{
public:
	static void Info(std::wstring_view Message);
	static void Warning(std::wstring_view Message);
	static void Error(std::wstring_view Message);

	FLog();
	~FLog();

private:
	void Write(std::wstring_view Prefix, std::wstring_view Message);

	HANDLE ConsoleOutput = nullptr;
};
