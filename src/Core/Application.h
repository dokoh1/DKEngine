#pragma once
//==============================================================================
// Application.h - 엔진의 최상위 객체 (Phase 1)
//
// 윈도우를 소유하고 게임 루프(Update/Render)를 돌린다.
// Phase 2 에서 여기에 D3D12 렌더러 초기화/렌더가 추가된다.
//==============================================================================
#include "Window.h"

namespace dk {

class Application
{
public:
    bool Init(HINSTANCE hInstance);
    int  Run();   // 게임 루프. 종료 코드를 반환.

private:
    void Update();
    void Render();

    Window m_window;
    unsigned long long m_frameCount = 0;
};

} // namespace dk
