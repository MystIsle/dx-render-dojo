#include "Core/FInput.h"

void FInput::KeyDown(unsigned int Key)
{
	Keys[Key] = true;
}

void FInput::KeyUp(unsigned int Key)
{
	Keys[Key] = false;
}
