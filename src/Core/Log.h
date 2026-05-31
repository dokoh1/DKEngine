#pragma once
//==============================================================================
// Log.h - 아주 단순한 디버그 로깅 헬퍼
//
// Windows 게임은 보통 콘솔이 없으므로(WinMain + Windows 서브시스템),
// OutputDebugStringA 로 Visual Studio "출력(Output)" 창에 로그를 찍는다.
// VS에서 F5로 디버깅 실행하면 출력 창에서 바로 볼 수 있다.
//==============================================================================
#include <Windows.h>
#include <cstdio>
#include <cstdarg>

namespace dk {

inline void LogImpl(const char* prefix, const char* fmt, ...)
{
    char body[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    char line[1152];
    snprintf(line, sizeof(line), "%s%s\n", prefix, body);

    OutputDebugStringA(line); // VS 출력 창
    printf("%s", line);        // 콘솔이 붙어 있으면 stdout 에도
}

} // namespace dk

#define DK_LOG(...)  ::dk::LogImpl("[DK] ", __VA_ARGS__)
#define DK_WARN(...) ::dk::LogImpl("[DK][WARN] ", __VA_ARGS__)
