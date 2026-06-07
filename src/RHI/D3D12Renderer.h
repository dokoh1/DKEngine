#pragma once
//==============================================================================
// D3D12Renderer.h - D3D12 화면 클리어 렌더러
// [Phase 2] D3D12 초기화, 백버퍼 clear/present, 리사이즈 처리를 위해 추가.
//==============================================================================
#include "Core/WindowsMinimal.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace dk {

class Window;

class D3D12Renderer
{
public:
    // [Phase 2] D3D12 COM 객체와 fence event 수명 관리.
    D3D12Renderer() = default;
    ~D3D12Renderer();

    // [Phase 2] D3D12 리소스 중복 소유를 막기 위해 복사 금지.
    D3D12Renderer(const D3D12Renderer&) = delete;
    D3D12Renderer& operator=(const D3D12Renderer&) = delete;

    // [Phase 2] 시작점이다.
    // Win32 창(HWND)에 붙는 스왑체인과 D3D12 명령 실행에 필요한 기본 객체들을 만든다.
    bool Initialize(const Window& window);

    // [Phase 2] 매 프레임 렌더 경로다.
    // 아직 draw call은 없고, 현재 백버퍼를 render target 상태로 바꾼 뒤 단색으로 clear하고 present한다.
    bool Render(int width, int height);

    // [Phase 2]
    // D3D12 리소스는 GPU가 아직 사용 중일 수 있으므로, 종료 전에 fence로 GPU 완료를 기다린다.
    void Shutdown();

private:
    // [Phase 2]
    // 더블 버퍼링을 사용한다.
    // 화면에 표시 중인 버퍼와 다음 프레임을 그릴 버퍼를 분리해 Present 흐름을 만든다.
    static constexpr unsigned int kFrameCount = 2;

    // [Phase 2] D3D12 초기화 단계를 작은 함수로 분리.
    bool EnableDebugLayer();
    bool CreateFactory();
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain(HWND hwnd, int width, int height);
    bool CreateRenderTargetViews();
    bool CreateFence();
    bool Resize(int width, int height);
    void ReleaseRenderTargets();
    void WaitForGpu();

    // [Phase 2]
    // DXGI Factory는 GPU 어댑터 열거와 스왑체인 생성을 담당한다.
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;

    // [Phase 2]
    // D3D12 Device는 리소스, 디스크립터 힙, 커맨드 객체를 만드는 중심 객체다.
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    // [Phase 2]
    // Command Queue는 CPU가 기록한 command list를 GPU에 제출하는 실행 큐다.
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

    // [Phase 2]
    // SwapChain은 백버퍼들을 관리하고 Present로 화면에 표시한다.
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

    // [Phase 2]
    // RTV Heap은 백버퍼를 render target으로 바라보기 위한 descriptor 배열이다.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;

    // [Phase 2]
    // CommandAllocator는 command list가 기록한 명령 메모리를 제공한다.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

    // [Phase 2]
    // CommandList는 resource barrier, clear 같은 GPU 명령을 기록하는 객체다.
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // [Phase 2]
    // Fence는 CPU가 GPU 작업 완료 시점을 확인하기 위한 동기화 객체다.
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

    // [Phase 2]
    // SwapChain에서 가져온 실제 백버퍼 리소스들이다.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[kFrameCount];

    // [Phase 2] Fence 대기와 현재 백버퍼/창 크기 추적 상태.
    HANDLE m_fenceEvent = nullptr;
    unsigned long long m_fenceValue = 0;
    unsigned int m_frameIndex = 0;
    unsigned int m_rtvDescriptorSize = 0;
    int m_width = 0;
    int m_height = 0;
    bool m_initialized = false;
};

} // namespace dk
