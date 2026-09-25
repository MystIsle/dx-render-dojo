#pragma once

class FApplication
{
public:
	FApplication() = default;
	FApplication(const FApplication& Other) = delete;
	FApplication& operator=(const FApplication& Other) = delete;

	void Update();
	void Render();
};
