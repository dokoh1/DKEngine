#include "D3D12Renderer.h"

#include "Core/Log.h"
#include "Core/Window.h"

#include <cwchar>
#include <cstdio>

namespace dk {
namespace {

// [Phase 2] D3D12/DXGI API 실패 지점을 로그에 남기기 위한 HRESULT 체크 헬퍼.
bool CheckHR(HRESULT hr, const char* what)
{
    if (SUCCEEDED(hr))
        return true;

    DK_WARN("%s 실패 (HRESULT=0x%08X)", what, static_cast<unsigned int>(hr));
    return false;
}

} // namespace

D3D12Renderer::~D3D12Renderer()
{
    // [Phase 2] 렌더러 객체가 사라질 때 GPU 리소스를 안전하게 정리.
    Shutdown();
}

bool D3D12Renderer::Initialize(const Window& window)
{
    // [Phase 2] 재초기화 시 기존 D3D12 리소스를 먼저 정리.
    if (m_initialized)
        Shutdown();

    m_width = window.Width();
    m_height = window.Height();

    if (m_width <= 0 || m_height <= 0)
    {
        DK_WARN("D3D12 초기화 실패: 창 크기가 유효하지 않음 (%dx%d)", m_width, m_height);
        return false;
    }

    DK_LOG("D3D12 초기화 시작 (%dx%d)", m_width, m_height);

    // [Phase 2]
    // D3D12 초기화는 의존 순서가 강하다.
    // Factory로 어댑터/스왑체인을 만들고, Device로 D3D12 객체를 만들며,
    // Command 객체와 SwapChain/RTV/Fence가 있어야 한 프레임을 clear/present할 수 있다.
    if (!EnableDebugLayer())
        return false;
    if (!CreateFactory())
        return false;
    if (!CreateDevice())
        return false;
    if (!CreateCommandObjects())
        return false;
    if (!CreateSwapChain(window.Handle(), m_width, m_height))
        return false;
    if (!CreateRenderTargetViews())
        return false;
    if (!CreateFence())
        return false;

    m_initialized = true;
    DK_LOG("D3D12 초기화 완료");
    return true;
}

bool D3D12Renderer::Render(int width, int height)
{
    // [Phase 2] 초기화되지 않은 렌더러는 렌더링할 수 없다.
    if (!m_initialized)
        return false;

    // [Phase 2] 최소화 중에는 창 크기가 0이 될 수 있으므로 렌더링을 건너뛴다.
    if (width <= 0 || height <= 0)
        return true;

    // [Phase 2]
    // Win32 창 크기가 바뀌면 SwapChain의 백버퍼도 같은 크기로 다시 만들어야 한다.
    // 기존 백버퍼를 GPU가 쓰고 있을 수 있으므로 Resize 내부에서 먼저 GPU 완료를 기다린다.
    if (width != m_width || height != m_height)
    {
        if (!Resize(width, height))
            return false;
    }

    // [Phase 2]
    // 매 프레임 새 명령을 기록하기 위해 allocator와 command list를 리셋한다.
    // Phase 2는 단순성을 위해 GPU 완료를 매 프레임 기다리므로 allocator 하나를 재사용할 수 있다.
    if (!CheckHR(m_commandAllocator->Reset(), "CommandAllocator Reset"))
        return false;

    if (!CheckHR(m_commandList->Reset(m_commandAllocator.Get(), nullptr), "CommandList Reset"))
        return false;

    // [Phase 2]
    // SwapChain 백버퍼는 Present 직후 표시 용도 상태다.
    // ClearRenderTargetView를 호출하려면 먼저 RenderTarget 상태로 명시 전환해야 한다.
    D3D12_RESOURCE_BARRIER toRenderTarget = {};
    toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRenderTarget.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &toRenderTarget);

    // [Phase 2]
    // RTV heap은 연속된 descriptor 배열이다.
    // 현재 백버퍼 인덱스에 맞춰 handle을 descriptor 크기만큼 이동해 해당 RTV를 선택한다.
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;

    // [Phase 2]
    // Phase 2의 실제 렌더링 작업은 이 clear 한 번이다.
    // 화면 전체가 이 색으로 보이면 Device/SwapChain/RTV/Command/Fence 흐름이 연결된 것이다.
    const float clearColor[] = { 0.08f, 0.16f, 0.28f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // [Phase 2]
    // Present는 RenderTarget 상태가 아니라 Present 상태의 백버퍼를 요구한다.
    // D3D12에서는 이런 상태 전이를 드라이버가 대신 해주지 않으므로 직접 기록해야 한다.
    D3D12_RESOURCE_BARRIER toPresent = {};
    toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &toPresent);

    // [Phase 2] command list 기록을 끝내고 GPU에 제출할 수 있는 상태로 닫는다.
    if (!CheckHR(m_commandList->Close(), "CommandList Close"))
        return false;

    // [Phase 2]
    // CPU가 기록한 command list를 GPU가 실행할 수 있도록 command queue에 제출한다.
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    // [Phase 2]
    // Present(1, 0)은 V-Sync에 맞춰 현재 백버퍼를 화면에 표시한다.
    if (!CheckHR(m_swapChain->Present(1, 0), "SwapChain Present"))
        return false;

    // [Phase 2]
    // Phase 2에서는 이해와 안전성을 우선해 매 프레임 GPU 완료를 기다린다.
    // 이후 Frame Resources를 도입하면 CPU/GPU 병렬성을 살리는 구조로 바꾼다.
    WaitForGpu();
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

void D3D12Renderer::Shutdown()
{
    // [Phase 2] 생성된 적 없는 렌더러는 정리할 D3D12 리소스가 없다.
    if (!m_initialized && !m_device)
        return;

    // [Phase 2]
    // ComPtr를 Reset하기 전에 GPU가 해당 리소스를 더 이상 쓰지 않는지 보장한다.
    WaitForGpu();

    ReleaseRenderTargets();

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    m_fence.Reset();
    m_commandList.Reset();
    m_commandAllocator.Reset();
    m_rtvHeap.Reset();
    m_swapChain.Reset();
    m_commandQueue.Reset();
    m_device.Reset();
    m_factory.Reset();

    m_width = 0;
    m_height = 0;
    m_frameIndex = 0;
    m_rtvDescriptorSize = 0;
    m_fenceValue = 0;
    m_initialized = false;
}

bool D3D12Renderer::EnableDebugLayer()
{
#ifdef _DEBUG
    // [Phase 2]
    // Debug Layer는 잘못된 resource state, 잘못된 descriptor 사용 같은 D3D12 실수를
    // Visual Studio 출력 창에 알려준다. Phase 2부터 켜두는 것이 이후 디버깅 비용을 줄인다.
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    if (FAILED(hr))
    {
        DK_WARN("D3D12 디버그 레이어를 사용할 수 없음 (HRESULT=0x%08X)", static_cast<unsigned int>(hr));
        return true;
    }

    debugController->EnableDebugLayer();
    DK_LOG("D3D12 디버그 레이어 활성화");
#endif
    return true;
}

bool D3D12Renderer::CreateFactory()
{
    unsigned int flags = 0;
#ifdef _DEBUG
    flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    // [Phase 2]
    // DXGI Factory는 D3D12 Device 자체를 만들지는 않지만,
    // 어떤 GPU 어댑터를 쓸지 고르고 HWND용 SwapChain을 만드는 출발점이다.
    HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));
    if (SUCCEEDED(hr))
        return true;

#ifdef _DEBUG
    DK_WARN("DXGI 디버그 팩토리 생성 실패, 일반 팩토리로 재시도 (HRESULT=0x%08X)",
            static_cast<unsigned int>(hr));
    return CheckHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)), "CreateDXGIFactory2");
#else
    return CheckHR(hr, "CreateDXGIFactory2");
#endif
}

bool D3D12Renderer::CreateDevice()
{
    Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;

    // [Phase 2]
    // 소프트웨어 어댑터를 피하고, D3D12 device 생성이 가능한 첫 하드웨어 어댑터를 선택한다.
    // Phase 2에서는 고성능 GPU 선택 정책보다 "동작하는 D3D12 경로"를 우선한다.
    for (unsigned int adapterIndex = 0;; ++adapterIndex)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        if (m_factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
        {
            selectedAdapter = adapter;

            char name[256] = {};
            std::snprintf(name, sizeof(name), "%ls", desc.Description);
            DK_LOG("D3D12 어댑터 선택: %s", name);
            break;
        }
    }

    if (!selectedAdapter)
    {
        DK_WARN("D3D12 지원 하드웨어 어댑터를 찾지 못함");
        return false;
    }

    // [Phase 2] 실제 D3D12 Device 생성.
    if (!CheckHR(D3D12CreateDevice(selectedAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)),
                 "D3D12CreateDevice"))
        return false;

    // [Phase 2]
    // 디버그 레이어/PIX에서 객체 이름이 보이면 캡처 분석이 쉬워진다.
    m_device->SetName(L"DKEngine D3D12 Device");
    return true;
}

bool D3D12Renderer::CreateCommandObjects()
{
    // [Phase 2]
    // DIRECT queue는 그래픽스/복사/컴퓨트 명령을 실행할 수 있는 기본 그래픽스 큐다.
    // Phase 2의 clear와 이후 Phase 3의 draw 모두 이 queue에 제출된다.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    if (!CheckHR(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)),
                 "CreateCommandQueue"))
        return false;
    m_commandQueue->SetName(L"DKEngine Direct Command Queue");

    // [Phase 2]
    // Allocator는 command list가 기록하는 명령 메모리의 저장소다.
    // GPU가 아직 이 메모리를 읽고 있으면 Reset하면 안 되므로 fence와 함께 관리한다.
    if (!CheckHR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                  IID_PPV_ARGS(&m_commandAllocator)),
                 "CreateCommandAllocator"))
        return false;
    m_commandAllocator->SetName(L"DKEngine Command Allocator");

    // [Phase 2]
    // Command list는 생성 직후 열린 상태다.
    // 렌더 루프에서 Reset해서 다시 열기 위해 초기화 단계에서는 한 번 Close해 둔다.
    if (!CheckHR(m_device->CreateCommandList(0,
                                             D3D12_COMMAND_LIST_TYPE_DIRECT,
                                             m_commandAllocator.Get(),
                                             nullptr,
                                             IID_PPV_ARGS(&m_commandList)),
                 "CreateCommandList"))
        return false;
    m_commandList->SetName(L"DKEngine Command List");

    return CheckHR(m_commandList->Close(), "Initial CommandList Close");
}

bool D3D12Renderer::CreateSwapChain(HWND hwnd, int width, int height)
{
    // [Phase 2]
    // SwapChain은 HWND에 붙는 백버퍼 묶음이다.
    // FLIP_DISCARD는 현대 DXGI의 권장 flip model이며, Present 후 이전 백버퍼 내용은 보존하지 않는다.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = static_cast<unsigned int>(width);
    swapChainDesc.Height = static_cast<unsigned int>(height);
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = kFrameCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
    if (!CheckHR(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(),
                                                   hwnd,
                                                   &swapChainDesc,
                                                   nullptr,
                                                   nullptr,
                                                   &swapChain1),
                 "CreateSwapChainForHwnd"))
        return false;

    // [Phase 2]
    // Alt+Enter 전체화면 전환을 DXGI 기본 동작에 맡기지 않는다.
    // 엔진이 나중에 직접 전체화면/창모드 정책을 관리하기 위한 선택이다.
    if (!CheckHR(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER),
                 "MakeWindowAssociation"))
        return false;

    if (!CheckHR(swapChain1.As(&m_swapChain), "IDXGISwapChain3 Query"))
        return false;

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12Renderer::CreateRenderTargetViews()
{
    // [Phase 2]
    // RTV descriptor heap은 "백버퍼 리소스를 render target으로 사용하는 방법"을 담는 핸들 배열이다.
    // 리소스 자체와 descriptor는 별개이므로 SwapChain에서 버퍼를 얻은 뒤 RTV를 만들어야 clear할 수 있다.
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = kFrameCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    if (!m_rtvHeap)
    {
        if (!CheckHR(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)),
                     "CreateDescriptorHeap(RTV)"))
            return false;
        m_rtvHeap->SetName(L"DKEngine RTV Heap");

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (unsigned int i = 0; i < kFrameCount; ++i)
    {
        if (!CheckHR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
                     "SwapChain GetBuffer"))
            return false;

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);

        wchar_t name[64] = {};
        std::swprintf(name, 64, L"DKEngine Back Buffer %u", i);
        m_renderTargets[i]->SetName(name);

        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    return true;
}

bool D3D12Renderer::CreateFence()
{
    // [Phase 2]
    // Fence는 CPU가 "GPU가 여기까지 실행했는지" 확인하는 카운터다.
    // event와 함께 쓰면 CPU가 GPU 완료 시점까지 효율적으로 기다릴 수 있다.
    if (!CheckHR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
                 "CreateFence"))
        return false;
    m_fence->SetName(L"DKEngine Fence");

    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        DK_WARN("CreateEventW 실패 (GetLastError=%lu)", GetLastError());
        return false;
    }

    return true;
}

bool D3D12Renderer::Resize(int width, int height)
{
    // [Phase 2]
    // ResizeBuffers는 기존 백버퍼를 모두 해제한 상태에서 호출해야 한다.
    // GPU가 백버퍼를 사용 중일 수도 있으므로 먼저 fence로 완료를 보장한다.
    WaitForGpu();
    ReleaseRenderTargets();

    DXGI_SWAP_CHAIN_DESC desc = {};
    if (!CheckHR(m_swapChain->GetDesc(&desc), "SwapChain GetDesc"))
        return false;

    if (!CheckHR(m_swapChain->ResizeBuffers(kFrameCount,
                                            static_cast<unsigned int>(width),
                                            static_cast<unsigned int>(height),
                                            desc.BufferDesc.Format,
                                            desc.Flags),
                 "SwapChain ResizeBuffers"))
        return false;

    m_width = width;
    m_height = height;
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    DK_LOG("D3D12 백버퍼 리사이즈: %dx%d", m_width, m_height);
    return CreateRenderTargetViews();
}

void D3D12Renderer::ReleaseRenderTargets()
{
    // [Phase 2] ResizeBuffers/Shutdown 전에 백버퍼 ComPtr 참조를 해제한다.
    for (unsigned int i = 0; i < kFrameCount; ++i)
        m_renderTargets[i].Reset();
}

void D3D12Renderer::WaitForGpu()
{
    // [Phase 2] 아직 동기화 객체가 준비되지 않은 초기/종료 경로에서는 할 일이 없다.
    if (!m_commandQueue || !m_fence || !m_fenceEvent)
        return;

    // [Phase 2]
    // queue에 fence signal 명령을 넣고, GPU가 그 지점까지 실행하면 fence 값이 올라간다.
    // CPU는 completed value를 확인하고 필요할 때 event로 대기한다.
    const unsigned long long fenceToWaitFor = ++m_fenceValue;
    if (FAILED(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor)))
        return;

    if (m_fence->GetCompletedValue() < fenceToWaitFor)
    {
        if (SUCCEEDED(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent)))
            WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

} // namespace dk
