# DKEngine 아키텍처 노트

이 문서는 코드가 늘어날 때 책임이 섞이지 않도록 잡아두는 기준이다.
초기 단계에서는 과한 추상화보다 **모듈 경계가 무너지지 않는 것**을 우선한다.

## 1. 현재 구조

```
src/
├─ main.cpp
├─ Core/
   ├─ Application.{h,cpp}
   ├─ Window.{h,cpp}
   ├─ Log.h
   └─ WindowsMinimal.h
└─ RHI/
   └─ D3D12Renderer.{h,cpp}
```

| 모듈 | 책임 |
|------|------|
| `main.cpp` | 프로그램 진입점. `Application` 생성과 실행만 담당 |
| `Core/Application` | 엔진 최상위 조율자. 창, 렌더러, 나중의 입력/씬을 연결 |
| `Core/Window` | Win32 창 생성, 메시지 처리, `HWND` 수명 관리 |
| `Core/Log` | 개발 중 로그 출력 |
| `Core/WindowsMinimal` | `Windows.h` 포함 정책 통일 |
| `RHI/D3D12Renderer` | D3D12 디바이스, 스왑체인, RTV, 커맨드 큐, 펜스 관리 |

## 2. Phase 2에서 추가한 구조

Phase 2부터 D3D12 세부 객체는 `Application`에 직접 넣지 않는다.

```
src/
├─ Core/
└─ RHI/
   ├─ D3D12Renderer.{h,cpp}
   ├─ Device.{h,cpp}          (Phase 4에서 분리 가능)
   ├─ SwapChain.{h,cpp}       (Phase 4에서 분리 가능)
   └─ CommandQueue.{h,cpp}    (Phase 4에서 분리 가능)
```

초기 Phase 2에서는 파일을 너무 잘게 나누지 않아도 된다. 다만 원칙은 유지한다:

- `Application`은 `renderer.Initialize(window)`, `renderer.Render()`, `renderer.Resize(...)`, `renderer.Shutdown()` 정도만 호출한다.
- `ID3D12Device`, `IDXGISwapChain`, `ID3D12CommandQueue`, `ID3D12Fence` 같은 COM 객체는 RHI가 소유한다.
- 창 크기 변경은 `Window`가 크기를 저장하고, `Application`이 렌더러에 리사이즈를 전달하는 방향으로 확장한다.
- 종료 시에는 렌더러가 먼저 GPU 작업을 `Flush`하고, 그 다음 창이 정리된다.

## 3. 수명 관리 원칙

- OS 핸들(`HWND` 등), COM 객체, GPU 리소스는 만든 객체가 정리 책임도 갖는다.
- 가능하면 raw `new/delete`를 쓰지 않는다. 값 멤버, `std::unique_ptr`, `Microsoft::WRL::ComPtr`를 우선한다.
- 실패 경로와 정상 종료 경로가 같은 정리 함수를 타게 만든다.
- `Application`에는 정책과 흐름을 두고, 플랫폼/API 세부 코드는 하위 모듈에 둔다.

## 4. 당장 피할 것

- `Application.cpp`에 D3D12 초기화 코드를 수백 줄 직접 쌓기
- Win32 메시지 처리 안에 게임 로직이나 렌더링 코드를 넣기
- Phase 2부터 완성형 엔진 추상화를 만들려고 파일을 과하게 쪼개기
- 디버그 레이어 경고를 "나중에" 처리하기
