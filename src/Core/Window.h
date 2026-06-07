#pragma once
//==============================================================================
// Window.h - Win32 윈도우 래퍼
// [Phase 1] 창 생성, 메시지 처리, 종료/리사이즈 상태 관리.
// [Phase 2] D3D12Renderer가 사용할 HWND와 클라이언트 크기 제공.
//==============================================================================
#include "WindowsMinimal.h"

namespace dk {

class Window
{
public:
    // [Phase 1] Win32 창 리소스 수명 관리.
    Window() = default;
    ~Window();

    // [Phase 1] HWND 중복 소유를 막기 위해 복사 금지.
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // [Phase 1] Win32 창 생성.
    bool Create(const wchar_t* title, int width, int height, HINSTANCE hInstance);

    // [Phase 1] Win32 창과 등록한 window class 정리.
    void Destroy();

    // [Phase 1] PeekMessage 기반 비블로킹 메시지 처리.
    bool ProcessMessages();

    // [Phase 2] D3D12Renderer가 스왑체인을 만들 때 필요한 OS 창 핸들이다.
    HWND Handle() const { return m_hwnd; }

    // [Phase 2] 스왑체인 백버퍼 크기와 리사이즈 기준으로 쓰는 클라이언트 영역 크기다.
    int  Width()  const { return m_width; }
    int  Height() const { return m_height; }

private:
    // [Phase 1] Win32 C 콜백을 C++ 객체 멤버 함수로 연결하는 함수들.
    static LRESULT CALLBACK WndProcSetup(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK WndProcThunk(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);

    // [Phase 1] Win32 창 핸들과 클래스 등록 상태.
    HWND       m_hwnd      = nullptr;
    HINSTANCE  m_hInstance = nullptr;
    bool       m_classRegistered = false;

    // [Phase 1] WM_SIZE에서 갱신되는 클라이언트 크기.
    // [Phase 2] D3D12Renderer의 SwapChain ResizeBuffers 기준으로 사용.
    int        m_width     = 0;
    int        m_height    = 0;

    // [Phase 1] RegisterClassExW/CreateWindowExW에서 공유하는 window class 이름.
    static constexpr const wchar_t* kClassName = L"DKEngineWindowClass";
};

} // namespace dk
