#pragma once
//==============================================================================
// Application.h - 엔진 실행 흐름 조율자
// [Phase 1] Window와 게임 루프 소유.
// [Phase 2] D3D12Renderer 소유와 렌더 호출 연결.
//==============================================================================
#include "Window.h"
#include "RHI/D3D12Renderer.h"

namespace dk {

class Application
{
public:
    // [Phase 1] 창과 기본 시스템 초기화.
    // [Phase 2] D3D12Renderer 초기화가 추가됨.
    bool Init(HINSTANCE hInstance);

    // [Phase 1] Win32 메시지 처리, Update, Render를 반복하는 게임 루프.
    // [Phase 2] 종료 전에 D3D12Renderer를 명시적으로 Shutdown하는 흐름이 추가됨.
    int  Run();

private:
    // [Phase 1] 임시 프레임 카운터 갱신.
    void Update();

    // [Phase 1] 빈 렌더 함수로 시작.
    // [Phase 2] D3D12Renderer::Render 호출로 백버퍼 clear/present 수행.
    void Render();

    // [Phase 1] Win32 창과 메시지 처리를 담당하는 객체.
    Window m_window;

    // [Phase 2] 핵심 연결점이다.
    // D3D12 세부 객체(Device, SwapChain, CommandQueue 등)는 Application에 직접 두지 않고
    // RHI 계층의 D3D12Renderer가 소유하게 해서 실행 흐름과 렌더링 API 구현을 분리한다.
    D3D12Renderer m_renderer;

    // [Phase 1] 게임 루프가 돌고 있는지 확인하기 위한 임시 카운터.
    unsigned long long m_frameCount = 0;
};

} // namespace dk
