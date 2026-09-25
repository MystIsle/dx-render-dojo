#pragma once

#include <array>

class FApplication
{
public:
	FApplication() = default;
	FApplication(const FApplication& Other) = delete;
	FApplication& operator=(const FApplication& Other) = delete;

	const std::array<float, 4>& GetClearColor() const { return ClearColor; }

	void Update();
	void Render();

private:
	std::array<float, 4> ClearColor = {0.5f, 0.5f, 0.5f, 1.0f};
};
