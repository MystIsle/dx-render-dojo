#include "Utility/Check.h"

#include <cstdint>
#include <cstdlib>
#include <format>
#include <string>

#include "Utility/FLog.h"

namespace
{
	std::wstring FormatFailure(std::wstring_view Prefix,
	                           std::wstring_view Expression,
	                           std::wstring_view Detail,
	                           std::wstring_view File,
	                           int Line)
	{
		std::wstring Message = std::format(L"{}{}", Prefix, Expression);
		if (Detail.empty() == false)
		{
			Message += std::format(L"\n    {}", Detail);
		}
		Message += std::format(L"\n    {}({})", File, Line);
		return Message;
	}
} // namespace

namespace Return
{
	std::wstring Describe(HRESULT Result)
	{
		return std::format(L"HRESULT 0x{:08X}", static_cast<std::uint32_t>(Result));
	}

	void ReportFailure(std::wstring_view Expression, std::wstring_view Detail, std::wstring_view File, int Line)
	{
		FLog::Error(FormatFailure(L"[CHECK FAILED] ", Expression, Detail, File, Line));
	}

	void ReportFatal(std::wstring_view Expression, std::wstring_view Detail, std::wstring_view File, int Line)
	{
		const std::wstring Message = FormatFailure(L"[CHECK FATAL] ", Expression, Detail, File, Line);
		FLog::Error(Message);

		MessageBoxW(nullptr, Message.c_str(), L"DxRenderDojo", MB_OK | MB_ICONERROR | MB_TASKMODAL);

		// 소멸자를 거치지 않고 끝낸다. 실패한 상태에서 정리 코드가 다시 실패할 수 있다.
		std::_Exit(1);
	}

	void ReportResurrection(std::string_view TypeName)
	{
		const std::wstring Name(TypeName.begin(), TypeName.end());
		FLog::Warning(std::format(L"해제한 싱글톤이 다시 만들어졌다 : {}", Name));
	}
} // namespace Return
