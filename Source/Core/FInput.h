#pragma once

#include <array>

class FInput
{
public:
	bool IsKeyDown(unsigned int Key) const { return Keys[Key]; }

	void KeyDown(unsigned int Key);
	void KeyUp(unsigned int Key);

private:
	std::array<bool, 256> Keys = {};
};
