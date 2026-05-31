#include "Window.h"
#include "Log.h"

namespace dk {

bool Window::Create(const wchar_t* title, int width, int height, HINSTANCE hInstance)
{
    m_hInstance = hInstance;
    m_width = width;
    m_height = height;

    // 1) 윈도우 클래스 등록 -------------------------------------------------
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW; // 크기 변경 시 다시 그림
    wc.lpfnWndProc   = &Window::WndProcSetup;   // 최초 메시지는 Setup 이 받음
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;

    if (!RegisterClassExW(&wc))
    {
        DK_WARN("RegisterClassExW 실패 (GetLastError=%lu)", GetLastError());
        return false;
    }

    // 2) 클라이언트 영역이 정확히 width x height 가 되도록 창 크기 보정 -----
    //    CreateWindow 의 크기는 테두리/타이틀바를 포함하므로 AdjustWindowRect 로 보정한다.
    RECT rect = { 0, 0, width, height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE);
    const int winW = rect.right - rect.left;
    const int winH = rect.bottom - rect.top;

    // 3) 창 생성 ------------------------------------------------------------
    //    마지막 인자(lpParam)로 this 를 넘기면 WM_NCCREATE 에서 받을 수 있다.
    m_hwnd = CreateWindowExW(
        0, kClassName, title, style,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, hInstance, this);

    if (!m_hwnd)
    {
        DK_WARN("CreateWindowExW 실패 (GetLastError=%lu)", GetLastError());
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    DK_LOG("윈도우 생성 완료 (%dx%d)", width, height);
    return true;
}

void Window::Destroy()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(kClassName, m_hInstance);
}

bool Window::ProcessMessages()
{
    // PeekMessage: 메시지가 없으면 즉시 반환(비블로킹). 게임 루프에 적합.
    // (GetMessage 는 메시지가 올 때까지 블로킹하므로 게임에는 부적합)
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false; // 종료 요청

        TranslateMessage(&msg);
        DispatchMessageW(&msg); // -> WndProc(여기선 WndProcThunk) 호출
    }
    return true;
}

//------------------------------------------------------------------------------
// WndProcSetup: 창 생성 직후 첫 메시지(WM_NCCREATE)에서 this 포인터를 꺼내
// HWND 의 사용자 데이터 슬롯에 저장하고, 이후 메시지는 Thunk 가 받도록 교체한다.
//------------------------------------------------------------------------------
LRESULT CALLBACK Window::WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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

//------------------------------------------------------------------------------
// WndProcThunk: 저장해 둔 this 를 꺼내 멤버 함수로 전달한다.
//------------------------------------------------------------------------------
LRESULT CALLBACK Window::WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return self->HandleMessage(hwnd, msg, wParam, lParam);
}

//------------------------------------------------------------------------------
// 실제 메시지 처리 (멤버 함수라 m_width 등에 자유롭게 접근 가능)
//------------------------------------------------------------------------------
LRESULT Window::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0); // 메시지 큐에 WM_QUIT 를 넣어 루프를 끝낸다
        return 0;

    case WM_SIZE:
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
