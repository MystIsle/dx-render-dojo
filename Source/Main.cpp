#include <DirectXMath.h>
#include <Windows.h>

#include "Core/FSystem.h"
#include "Utility/Check.h"
#include "Utility/FLog.h"

int WINAPI wWinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PrevInstance, _In_ PWSTR CmdLine, _In_ int ShowCmd)
{
	// DirectXMath 가 쓰는 SIMD 명령을 이 CPU 가 지원하는지 먼저 본다.
	CHECK_FATAL(DirectX::XMVerifyCPUSupport());

	// 디버그 빌드의 로그 콘솔을 창보다 먼저 연다. 창 뒤에 열리면 콘솔이 앞에 와서 키 입력을 가져간다.
	FLog::Prepare();

	// FSystem 이 가진 것을 확실한 시점에 정리하려고 명시적으로 만들고 지운다.
	FSystem::Prepare();
	FSystem::Get().Initialize(ShowCmd);

	const int ExitCode = FSystem::Get().Run();
	FSystem::Release();
	return ExitCode;
}
