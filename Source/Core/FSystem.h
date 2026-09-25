#pragma once

#include <Windows.h>

#include "Core/FDisplaySettings.h"
#include "Core/FWindow.h"
#include "Utility/TSingleton.h"

class FSystem : public TSingleton<FSystem>
{
public:
	void Initialize(int ShowCmd);

	int Run();

private:
	FDisplaySettings Display;
	FWindow Window;
};
