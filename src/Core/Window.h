#pragma once
//==============================================================================
// Window.h - Win32 윈도우 래퍼 (Phase 1)
//
// 운영체제 창 하나를 만들고, 게임 루프에서 메시지를 처리하는 책임만 담당한다.
// 렌더링/게임 로직은 여기에 두지 않는다(관심사 분리).
//==============================================================================
#include <Windows.h>

namespace dk {

class Window
{
public:
    // 창을 생성한다. 실패 시 false.
    bool Create(const wchar_t* title, int width, int height, HINSTANCE hInstance);
    void Destroy();

    // 큐에 쌓인 윈도우 메시지를 모두 처리한다(비블로킹).
    // 종료(WM_QUIT)가 요청되면 false 를 반환한다.
    bool ProcessMessages();

    HWND Handle() const { return m_hwnd; }
    int  Width()  const { return m_width; }
    int  Height() const { return m_height; }

private:
    // WndProc 는 C 함수 포인터라서 멤버 함수를 직접 등록할 수 없다.
    // 그래서 "static 함수에서 HWND 에 저장해 둔 this 포인터를 꺼내
    // 멤버 함수(HandleMessage)로 넘기는" 표준 패턴을 사용한다.
    static LRESULT CALLBACK WndProcSetup(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK WndProcThunk(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);

    HWND       m_hwnd      = nullptr;
    HINSTANCE  m_hInstance = nullptr;
    int        m_width     = 0;
    int        m_height    = 0;

    static constexpr const wchar_t* kClassName = L"DKEngineWindowClass";
};

} // namespace dk
