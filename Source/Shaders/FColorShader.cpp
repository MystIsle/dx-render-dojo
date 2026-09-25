#include "Shaders/FColorShader.h"

#include <array>
#include <d3dcompiler.h>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "Utility/Check.h"
#include "Utility/FLog.h"
#include "Utility/FPaths.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	ComPtr<ID3DBlob> CompileShader(const std::filesystem::path& Path, const char* EntryPoint, const char* Target)
	{
		UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS;
		// 디버그 빌드는 그래픽 디버거(RenderDoc·PIX)에서 HLSL 을 줄 단위로 따라갈 수 있게 컴파일한다.
#ifndef NDEBUG
		Flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ComPtr<ID3DBlob> Code;
		ComPtr<ID3DBlob> Errors;
		const HRESULT CompileResult = D3DCompileFromFile(
		    Path.c_str(), nullptr, nullptr, EntryPoint, Target, Flags, 0, Code.GetAddressOf(), Errors.GetAddressOf());

		// 컴파일러 문자열 속 경로는 한글이 ? 로 바뀔 수 있어 경로를 따로 남긴다.
		if (FAILED(CompileResult))
		{
			FLog::Error(std::format(L"셰이더 컴파일 실패 : {}", Path.wstring()));
		}

		// 성공해도 경고가 담겨 올 수 있다.
		if (Errors != nullptr)
		{
			std::string_view Text(static_cast<const char*>(Errors->GetBufferPointer()));
			Text = Text.substr(0, Text.find_last_not_of("\r\n") + 1);

			const int TextLength = static_cast<int>(Text.size());
			std::wstring Message(MultiByteToWideChar(CP_ACP, 0, Text.data(), TextLength, nullptr, 0), L'\0');
			MultiByteToWideChar(CP_ACP, 0, Text.data(), TextLength, Message.data(), static_cast<int>(Message.size()));

			if (SUCCEEDED(CompileResult))
			{
				FLog::Warning(Message);
			}
			else
			{
				FLog::Error(Message);
			}
		}

		CHECK_FATAL(CompileResult);
		return Code;
	}
} // namespace

void FColorShader::Initialize(ID3D11Device* Device)
{
	// 빌드가 셰이더 파일을 exe 옆 Shaders 폴더로 복사한다.
	const std::filesystem::path ShaderDirectory = FPaths::GetExecutableDirectory() / L"Shaders";
	const ComPtr<ID3DBlob> VertexCode = CompileShader(ShaderDirectory / L"Color.vs", "ColorVertexShader", "vs_5_0");
	const ComPtr<ID3DBlob> PixelCode = CompileShader(ShaderDirectory / L"Color.ps", "ColorPixelShader", "ps_5_0");

	CHECK_FATAL(Device->CreateVertexShader(
	    VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), nullptr, VertexShader.GetAddressOf()));
	CHECK_FATAL(Device->CreatePixelShader(
	    PixelCode->GetBufferPointer(), PixelCode->GetBufferSize(), nullptr, PixelShader.GetAddressOf()));

	constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> InputElements = {{
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	}};
	CHECK_FATAL(Device->CreateInputLayout(InputElements.data(),
	                                      static_cast<UINT>(InputElements.size()),
	                                      VertexCode->GetBufferPointer(),
	                                      VertexCode->GetBufferSize(),
	                                      InputLayout.GetAddressOf()));

	// 상수 버퍼 크기는 16 바이트의 배수여야 한다.
	static_assert(sizeof(FMatrixBuffer) % 16 == 0);
	D3D11_BUFFER_DESC MatrixBufferDesc = {};
	MatrixBufferDesc.ByteWidth = sizeof(FMatrixBuffer);
	MatrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	MatrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	MatrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	CHECK_FATAL(Device->CreateBuffer(&MatrixBufferDesc, nullptr, MatrixBuffer.GetAddressOf()));
}

void FColorShader::Render(ID3D11DeviceContext* Context,
                          UINT IndexCount,
                          const XMMATRIX& World,
                          const XMMATRIX& View,
                          const XMMATRIX& Projection) const
{
	// HLSL 은 행렬을 열 우선으로 읽는다. DirectXMath 의 행 우선 행렬을 전치해서 넣는다.
	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	CHECK_FATAL(Context->Map(MatrixBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped));
	FMatrixBuffer* Matrices = static_cast<FMatrixBuffer*>(Mapped.pData);
	XMStoreFloat4x4(&Matrices->World, XMMatrixTranspose(World));
	XMStoreFloat4x4(&Matrices->View, XMMatrixTranspose(View));
	XMStoreFloat4x4(&Matrices->Projection, XMMatrixTranspose(Projection));
	Context->Unmap(MatrixBuffer.Get(), 0);

	Context->IASetInputLayout(InputLayout.Get());
	Context->VSSetShader(VertexShader.Get(), nullptr, 0);
	Context->VSSetConstantBuffers(0, 1, MatrixBuffer.GetAddressOf());
	Context->PSSetShader(PixelShader.Get(), nullptr, 0);
	Context->DrawIndexed(IndexCount, 0, 0);
}
