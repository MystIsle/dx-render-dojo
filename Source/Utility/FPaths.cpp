#include "Utility/FPaths.h"

#include <Windows.h>
#include <string>

#include "Utility/Check.h"

std::filesystem::path FPaths::GetExecutableDirectory()
{
	// Windows 경로의 최대 길이(32767 자)만큼 잡아 긴 경로도 받는다.
	std::wstring Buffer(32768, L'\0');
	const DWORD Length = GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
	CHECK_FATAL(Length != 0 && Length < Buffer.size());
	Buffer.resize(Length);
	return std::filesystem::path(Buffer).parent_path();
}
