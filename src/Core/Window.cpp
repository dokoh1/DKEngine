#include "Window.h"
#include "Log.h"

namespace dk {

Window::~Window()
{
    // [Phase 1] Window 객체가 사라질 때 Win32 리소스도 함께 정리.
    Destroy();
}

bool Window::Create(const wchar_t* title, int width, int height, HINSTANCE hInstance)
{
    // [Phase 1] 같은 객체로 창을 다시 만들 경우 기존 창을 먼저 정리.
    Destroy();

    m_hInstance = hInstance;
    m_width = width;
    m_height = height;

    // [Phase 1] Win32 창 생성에 필요한 window class 등록.
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &Window::WndProcSetup;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;

    if (!RegisterClassExW(&wc))
    {
        DK_WARN("RegisterClassExW 실패 (GetLastError=%lu)", GetLastError());
        return false;
    }
    m_classRegistered = true;

    // [Phase 1] 요청한 크기를 클라이언트 영역 기준으로 맞추기 위한 외곽 크기 보정.
    // [Phase 2] 이 클라이언트 크기가 SwapChain 백버퍼 초기 크기와 일치해야 한다.
    RECT rect = { 0, 0, width, height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!AdjustWindowRect(&rect, style, FALSE))
    {
        DK_WARN("AdjustWindowRect 실패 (GetLastError=%lu)", GetLastError());
        Destroy();
        return false;
    }

    const int winW = rect.right - rect.left;
    const int winH = rect.bottom - rect.top;

    // [Phase 1] 실제 HWND 생성. 마지막 인자의 this는 WndProcSetup에서 회수한다.
    // [Phase 2] 생성된 HWND는 D3D12Renderer::CreateSwapChainForHwnd에 전달된다.
    m_hwnd = CreateWindowExW(
        0, kClassName, title, style,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, hInstance, this);

    if (!m_hwnd)
    {
        DK_WARN("CreateWindowExW 실패 (GetLastError=%lu)", GetLastError());
        Destroy();
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    DK_LOG("윈도우 생성 완료 (%dx%d)", width, height);
    return true;
}

void Window::Destroy()
{
    // [Phase 1] HWND가 살아 있으면 파괴 메시지 흐름을 발생시켜 창을 닫는다.
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    // [Phase 1] RegisterClassExW로 등록한 window class를 해제한다.
    if (m_classRegistered && m_hInstance)
    {
        if (!UnregisterClassW(kClassName, m_hInstance))
            DK_WARN("UnregisterClassW 실패 (GetLastError=%lu)", GetLastError());
        m_classRegistered = false;
    }

    m_hInstance = nullptr;
}

bool Window::ProcessMessages()
{
    // [Phase 1] 게임 루프가 멈추지 않도록 PeekMessage로 쌓인 메시지만 처리한다.
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

LRESULT CALLBACK Window::WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // [Phase 1] CreateWindowExW에서 넘긴 this를 HWND 사용자 데이터에 저장하는 1회성 연결 단계.
    if (msg == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self   = reinterpret_cast<Window*>(create->lpCreateParams);

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC,  reinterpret_cast<LONG_PTR>(&Window::WndProcThunk));

        return self->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // [Phase 1] HWND에 저장해 둔 Window*를 꺼내 실제 멤버 함수로 메시지를 전달.
    auto* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    return self->HandleMessage(hwnd, msg, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // [Phase 1] 종료/리사이즈/ESC 입력만 직접 처리하고 나머지는 기본 Win32 처리에 맡긴다.
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_NCDESTROY:
        // [Phase 1] 창이 완전히 파괴될 때 HWND에 저장한 Window* 연결을 끊는다.
        if (hwnd == m_hwnd)
            m_hwnd = nullptr;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_SIZE:
        // [Phase 1] 새 클라이언트 크기를 저장.
        // [Phase 2] 렌더러가 이 값을 보고 스왑체인 백버퍼를 리사이즈한다.
        m_width  = LOWORD(lParam);
        m_height = HIWORD(lParam);
        DK_LOG("리사이즈: %dx%d", m_width, m_height);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DK_LOG("ESC 입력 -> 종료");
            DestroyWindow(hwnd);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace dk
