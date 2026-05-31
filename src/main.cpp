//==============================================================================
// main.cpp - 프로그램 진입점 (Phase 1)
//
// Windows GUI 앱의 진입점은 main 이 아니라 wWinMain 이다.
// (콘솔 앱은 main, GUI 앱은 (w)WinMain)
//==============================================================================
#include "Core/Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    PWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    dk::Application app;
    if (!app.Init(hInstance))
        return -1;

    return app.Run();
}
