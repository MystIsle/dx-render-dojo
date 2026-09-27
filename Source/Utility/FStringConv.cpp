#include "Utility/FStringConv.h"

#include <Windows.h>

std::wstring FStringConv::ToWide(std::string_view Text)
{
	// 코드 페이지를 모르는 좁은 문자열은 시스템 코드 페이지(CP_ACP)로 읽는다.
	const int TextLength = static_cast<int>(Text.size());
	std::wstring Wide(MultiByteToWideChar(CP_ACP, 0, Text.data(), TextLength, nullptr, 0), L'\0');
	MultiByteToWideChar(CP_ACP, 0, Text.data(), TextLength, Wide.data(), static_cast<int>(Wide.size()));
	return Wide;
}
