#pragma once
//==============================================================================
// WindowsMinimal.h - Windows.h 포함 정책
// [Phase 1] Win32 창 코드에서 Windows.h를 일관된 설정으로 포함하기 위해 추가.
// [Phase 2] D3D12/DXGI 코드도 같은 Windows 타입(HWND, HANDLE 등)을 공유.
//==============================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
