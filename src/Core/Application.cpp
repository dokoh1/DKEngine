#include "Application.h"
#include "Log.h"

namespace dk {

bool Application::Init(HINSTANCE hInstance)
{
    DK_LOG("DKEngine 시작");
    if (!m_window.Create(L"DKEngine - Phase 1", 1280, 720, hInstance))
        return false;
    return true;
}

int Application::Run()
{
    // 게임 루프의 가장 기본 형태:
    //   메시지 처리 -> 업데이트 -> 렌더 를 종료될 때까지 반복.
    // Phase 9 에서 여기에 정밀 타이머와 고정 타임스텝이 들어간다.
    while (m_window.ProcessMessages())
    {
        Update();
        Render();
    }

    DK_LOG("게임 루프 종료 (총 %llu 프레임)", m_frameCount);
    return 0;
}

void Application::Update()
{
    ++m_frameCount;

    // 아직 보여줄 게 없으니, 약 1초(가정)에 한 번 살아있다는 로그만 남긴다.
    // (정확한 시간 측정은 Phase 9 의 타이머가 담당)
    if (m_frameCount % 200 == 0)
        DK_LOG("동작 중... frame=%llu", m_frameCount);
}

void Application::Render()
{
    // Phase 2 에서 이 자리에 D3D12 화면 클리어 + Present 가 들어간다.
}

} // namespace dk
