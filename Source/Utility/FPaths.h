#pragma once

#include <filesystem>

class FPaths
{
public:
	static std::filesystem::path GetExecutableDirectory();
};
