//==============================================================================
// main.cpp - 프로그램 진입점
// [Phase 1] Win32 GUI 앱의 wWinMain 진입점으로 추가.
//==============================================================================
#include "Core/Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    PWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    // [Phase 1] 엔진 최상위 Application 생성과 실행 흐름.
    dk::Application app;
    if (!app.Init(hInstance))
        return -1;

    return app.Run();
}
