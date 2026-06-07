#include "Application.h"
#include "Log.h"

namespace dk {

bool Application::Init(HINSTANCE hInstance)
{
    // [Phase 1] GUI 앱에서도 로그를 볼 수 있게 Debug 콘솔을 준비.
    dk::InitConsole();
    DK_LOG("DKEngine 시작");

    // [Phase 1] D3D12가 붙을 대상 Win32 창 생성.
    if (!m_window.Create(L"DKEngine - Phase 2", 1280, 720, hInstance))
        return false;

    // [Phase 2]
    // D3D12 스왑체인은 특정 HWND에 연결된다.
    // 따라서 Win32 창을 먼저 만든 뒤, 그 창 핸들과 클라이언트 크기를 이용해 렌더러를 초기화한다.
    if (!m_renderer.Initialize(m_window))
        return false;

    return true;
}

int Application::Run()
{
    // [Phase 1] 메시지 처리 -> 업데이트 -> 렌더를 반복하는 기본 게임 루프.
    while (m_window.ProcessMessages())
    {
        Update();
        Render();
    }

    DK_LOG("게임 루프 종료 (총 %llu 프레임)", m_frameCount);

    // [Phase 2]
    // 스왑체인과 백버퍼는 HWND와 연결된 GPU 리소스다.
    // 창이 사라지기 전에 GPU 작업을 끝까지 기다리고 렌더러 리소스를 먼저 정리한다.
    m_renderer.Shutdown();
    return 0;
}

void Application::Update()
{
    // [Phase 1] 아직 타이머가 없으므로 프레임 카운트만 임시로 갱신.
    ++m_frameCount;

    if (m_frameCount % 200 == 0)
        DK_LOG("동작 중... frame=%llu", m_frameCount);
}

void Application::Render()
{
    // [Phase 2] Application은 그리기 세부 과정을 알지 않는다.
    // 현재 창 크기만 넘기고, 백버퍼 리사이즈와 clear/present는 D3D12Renderer가 처리한다.
    if (!m_renderer.Render(m_window.Width(), m_window.Height()))
    {
        DK_WARN("렌더링 실패 -> 종료 요청");
        PostQuitMessage(1);
    }
}

} // namespace dk
