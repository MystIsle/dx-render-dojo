#include "Graphics/FD3D11Graphics.h"

#include <cstddef>
#include <d3d11sdklayers.h>
#include <dxgi1_6.h>
#include <format>
#include <string>
#include <vector>

#include "Utility/Check.h"
#include "Utility/FLog.h"
#include "Utility/FStringConv.h"

using Microsoft::WRL::ComPtr;

FD3D11Graphics::~FD3D11Graphics()
{
	// Initialize 가 건 훅이 사라진 객체를 부르지 않게 푼다.
	::Return::SetFatalHook(nullptr);
}

void FD3D11Graphics::Initialize(HWND WindowHandle, int Width, int Height, bool bEnableVSync)
{
	// 치명 실패로 끝나기 직전에 쌓인 디버그 메시지와 장치 제거 원인을 로그로 남긴다.
	::Return::SetFatalHook([this] { OnFatal(); });

	bVSync = bEnableVSync;

	UINT FactoryFlags = 0;
#ifndef NDEBUG
	// DXGI 디버그 큐는 D3D11 메시지까지 모은다.
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(InfoQueue.ReleaseAndGetAddressOf()))))
	{
		FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		// 디버거가 붙어 있으면 에러를 낸 호출 안에서 멈춘다.
		if (IsDebuggerPresent())
		{
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		}
	}
#endif

	ComPtr<IDXGIFactory2> Factory;
	CHECK_FATAL(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(Factory.ReleaseAndGetAddressOf())));

	ComPtr<IDXGIAdapter1> Adapter;
	ComPtr<IDXGIFactory6> Factory6;
	if (SUCCEEDED(Factory.As(&Factory6)))
	{
		CHECK_FATAL(Factory6->EnumAdapterByGpuPreference(
		    0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(Adapter.ReleaseAndGetAddressOf())));
	}
	else
	{
		CHECK_FATAL(Factory->EnumAdapters1(0, Adapter.ReleaseAndGetAddressOf()));
	}

	DXGI_ADAPTER_DESC1 AdapterDesc = {};
	CHECK_FATAL(Adapter->GetDesc1(&AdapterDesc));
	FLog::Info(std::format(
	    L"그래픽 카드 : {} ({} MB)", AdapterDesc.Description, AdapterDesc.DedicatedVideoMemory / 1024 / 1024));

	UINT Flags = 0;
#ifndef NDEBUG
	// 디버그 레이어는 Windows 선택적 기능 "그래픽 도구" 에 들어 있다.
	// 없는 PC 에서 이 플래그를 주면 디바이스 생성이 실패한다.
	const HRESULT LayerProbe = D3D11CreateDevice(nullptr,
	                                             D3D_DRIVER_TYPE_NULL,
	                                             nullptr,
	                                             D3D11_CREATE_DEVICE_DEBUG,
	                                             nullptr,
	                                             0,
	                                             D3D11_SDK_VERSION,
	                                             nullptr,
	                                             nullptr,
	                                             nullptr);
	if (SUCCEEDED(LayerProbe))
	{
		Flags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	else
	{
		FLog::Warning(L"D3D 디버그 레이어가 없어 끄고 진행한다. Windows 선택적 기능의 그래픽 도구를 설치하면 켜진다");
	}
#endif

	const D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	CHECK_FATAL(D3D11CreateDevice(Adapter.Get(),
	                              D3D_DRIVER_TYPE_UNKNOWN,
	                              nullptr,
	                              Flags,
	                              &FeatureLevel,
	                              1,
	                              D3D11_SDK_VERSION,
	                              Device.ReleaseAndGetAddressOf(),
	                              nullptr,
	                              Context.ReleaseAndGetAddressOf()));

#ifndef NDEBUG
	// 메시지는 FlushDebugMessages 가 FLog 로 옮긴다. D3D11 런타임이 출력 창에 직접 쓰면 두 번 찍혀서 끈다.
	// DXGI 런타임의 출력은 이 방법으로 꺼지지 않는다.
	ComPtr<ID3D11InfoQueue> D3DInfoQueue;
	if (SUCCEEDED(Device.As(&D3DInfoQueue)))
	{
		D3DInfoQueue->SetMuteDebugOutput(TRUE);

		if (IsDebuggerPresent())
		{
			D3DInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			D3DInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		}
	}
#endif

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width = static_cast<UINT>(Width);
	SwapChainDesc.Height = static_cast<UINT>(Height);
	SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	CHECK_FATAL(Factory->CreateSwapChainForHwnd(
	    Device.Get(), WindowHandle, &SwapChainDesc, nullptr, nullptr, SwapChain.ReleaseAndGetAddressOf()));

	// DXGI 는 기본으로 Alt+Enter 에 독점 전체 화면 전환을 건다. 이 앱은 테두리 없는 창만 쓴다.
	CHECK_FATAL(Factory->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER));

	CreateSizeDependentResources();
	FlushDebugMessages();
}

void FD3D11Graphics::Resize(int Width, int Height)
{
	// 백 버퍼를 가리키는 뷰가 남아 있으면 ResizeBuffers 가 실패한다. Flush 는 D3D 가 미뤄 둔 해제를 지금 끝낸다.
	RenderTargetView.Reset();
	Context->Flush();

	CHECK_FATAL(
	    SwapChain->ResizeBuffers(0, static_cast<UINT>(Width), static_cast<UINT>(Height), DXGI_FORMAT_UNKNOWN, 0));
	CreateSizeDependentResources();
}

void FD3D11Graphics::BeginScene()
{
	// ClearRenderTargetView 는 float 4개 배열을 받는다. DXGI_RGBA 는 r·g·b·a 를 이 순서로 이어 둔다.
	Context->ClearRenderTargetView(RenderTargetView.Get(), &ClearColor.r);
}

void FD3D11Graphics::EndScene()
{
	CHECK_FATAL(SwapChain->Present(bVSync ? 1 : 0, 0));
	FlushDebugMessages();
}

void FD3D11Graphics::CreateSizeDependentResources()
{
	ComPtr<ID3D11Texture2D> BackBuffer;
	CHECK_FATAL(SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.ReleaseAndGetAddressOf())));
	CHECK_FATAL(Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RenderTargetView.ReleaseAndGetAddressOf()));
}

void FD3D11Graphics::FlushDebugMessages()
{
	if (InfoQueue == nullptr)
	{
		return;
	}

	const UINT64 Count = InfoQueue->GetNumStoredMessagesAllowedByRetrievalFilters(DXGI_DEBUG_ALL);
	for (UINT64 Index = 0; Index < Count; ++Index)
	{
		SIZE_T Length = 0;
		CHECK_RETURN(InfoQueue->GetMessage(DXGI_DEBUG_ALL, Index, nullptr, &Length));

		std::vector<std::byte> Buffer(Length);
		DXGI_INFO_QUEUE_MESSAGE* Message = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(Buffer.data());
		CHECK_RETURN(InfoQueue->GetMessage(DXGI_DEBUG_ALL, Index, Message, &Length));

		const wchar_t* Source = (Message->Producer == DXGI_DEBUG_DXGI) ? L"[DXGI] " : L"[D3D] ";
		const std::wstring Text = Source + FStringConv::ToWide(Message->pDescription);

		switch (Message->Severity)
		{
		case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION:
		case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:
			FLog::Error(Text);
			break;
		case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:
			FLog::Warning(Text);
			break;
		default:
			FLog::Info(Text);
			break;
		}
	}

	InfoQueue->ClearStoredMessages(DXGI_DEBUG_ALL);
}

void FD3D11Graphics::OnFatal()
{
	FlushDebugMessages();

	// Present·ResizeBuffers 가 돌려주는 장치 제거 코드만으로는 원인을 알 수 없다.
	if (Device != nullptr)
	{
		const HRESULT RemovedReason = Device->GetDeviceRemovedReason();
		if (FAILED(RemovedReason))
		{
			FLog::Error(std::format(L"장치 제거 원인 : {}", ::Return::Describe(RemovedReason)));
		}
	}
}
