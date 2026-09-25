#pragma once

#include <Windows.h>
#include <string>
#include <string_view>

namespace Return
{
	inline bool Succeeded(HRESULT Result)
	{
		return SUCCEEDED(Result);
	}

	inline bool Succeeded(bool bResult)
	{
		return bResult;
	}

	template <typename T> bool Succeeded(T* Pointer)
	{
		return Pointer != nullptr;
	}

	std::wstring Describe(HRESULT Result);

	inline std::wstring Describe([[maybe_unused]] bool bResult)
	{
		return {};
	}

	template <typename T> std::wstring Describe([[maybe_unused]] T* Pointer)
	{
		return {};
	}

	// 정의는 Check.cpp 에 있다. 여기서 FLog.h 를 포함하면 TSingleton.h 를 거쳐 되돌아온다.
	void ReportFailure(std::wstring_view Expression, std::wstring_view Detail, std::wstring_view File, int Line);
	[[noreturn]] void ReportFatal(std::wstring_view Expression,
	                              std::wstring_view Detail,
	                              std::wstring_view File,
	                              int Line);

	void ReportResurrection(std::string_view TypeName);
} // namespace Return

#define CHECK_RETURN(Expression, ...)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		const auto CheckResult = (Expression);                                                                         \
		if (::Return::Succeeded(CheckResult) == false)                                                                 \
		{                                                                                                              \
			::Return::ReportFailure(L"" #Expression, ::Return::Describe(CheckResult), L"" __FILE__, __LINE__);         \
			return __VA_ARGS__;                                                                                        \
		}                                                                                                              \
	} while (false)

// 계속할 수 없는 실패는 그 자리에서 멈춘다. 디버거가 붙어 있으면 이 줄에서 먼저 멈춘다.
#define CHECK_FATAL(Expression)                                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		const auto CheckResult = (Expression);                                                                         \
		if (::Return::Succeeded(CheckResult) == false)                                                                 \
		{                                                                                                              \
			if (IsDebuggerPresent())                                                                                   \
			{                                                                                                          \
				__debugbreak();                                                                                        \
			}                                                                                                          \
			::Return::ReportFatal(L"" #Expression, ::Return::Describe(CheckResult), L"" __FILE__, __LINE__);           \
		}                                                                                                              \
	} while (false)
