#pragma once
//==============================================================================
// Log.h - 디버그 로그 헬퍼
// [Phase 1] Win32 창/게임 루프 상태 확인용으로 추가.
// [Phase 2] D3D12 초기화/렌더링 실패 지점 확인에 사용.
//==============================================================================
#include "WindowsMinimal.h"
#include <cstdio>
#include <cstdarg>

namespace dk {

inline void InitConsole()
{
#ifdef _DEBUG
    // [Phase 1] Windows subsystem 앱에서도 stdout/stderr 로그를 보기 위한 Debug 콘솔.
    if (AllocConsole())
    {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleTitleW(L"DKEngine - Debug Console");
    }
#endif
}

inline void LogImpl(const char* prefix, const char* fmt, ...)
{
    // [Phase 1] printf 스타일 로그를 한 줄 문자열로 만든 뒤 출력.
    char body[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    char line[1152];
    snprintf(line, sizeof(line), "%s%s\n", prefix, body);

    // [Phase 2] D3D12 초기화 실패는 Visual Studio 출력 창과 Debug 콘솔 양쪽에서 바로 확인할 수 있게 한다.
    OutputDebugStringA(line);
    printf("%s", line);
    fflush(stdout);
}

} // namespace dk

#define DK_LOG(...)  ::dk::LogImpl("[DK] ", __VA_ARGS__)
#define DK_WARN(...) ::dk::LogImpl("[DK][WARN] ", __VA_ARGS__)
