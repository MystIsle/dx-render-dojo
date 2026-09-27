#pragma once

#include <Windows.h>

#include "Core/FWindow.h"
#include "Utility/TSingleton.h"

class FSystem : public TSingleton<FSystem>
{
public:
	void Initialize(int ShowCmd);

	int Run();

private:
	FWindow Window;
};
