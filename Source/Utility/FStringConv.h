#pragma once

#include <string>
#include <string_view>

class FStringConv
{
public:
	static std::wstring ToWide(std::string_view Text);
};
