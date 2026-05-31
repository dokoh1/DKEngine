# Phase 1 — Win32 윈도우 & 게임 루프

## 1. 이번 Phase의 목표
운영체제 위에 **창 하나를 띄우고**, 그 창이 닫힐 때까지 도는 **게임 루프의 뼈대**를 만든다.
아직 그림(D3D)은 없다. "창이 뜨고, 입력/리사이즈에 반응하고, 깔끔히 종료된다"가 전부다.

이 단계가 중요한 이유: **앞으로 모든 렌더링은 이 창의 `HWND`(창 핸들) 위에서 일어난다.**
Phase 2의 D3D12 스왑체인도 결국 이 `HWND`에 화면을 붙인다.

---

## 2. 무엇을 만들었나

| 파일 | 역할 |
|------|------|
| `src/main.cpp` | 진입점 `wWinMain`. `Application`을 만들고 실행만 한다. |
| `src/Core/Application.{h,cpp}` | 엔진 최상위. 창을 소유하고 **게임 루프(Update/Render)** 를 돌린다. |
| `src/Core/Window.{h,cpp}` | **Win32 창 생성 + 메시지 처리**만 담당. |
| `src/Core/Log.h` | `OutputDebugString` 기반 디버그 로깅. |

설계 원칙: **관심사 분리**. "창을 다루는 일"(Window)과 "게임을 굴리는 일"(Application)을 나눴다.
Phase 2에서 렌더러가 추가돼도 `Application`이 "창 + 렌더러"를 조율하는 구조가 유지된다.

---

## 3. 왜 이렇게 했나 (의도)

### 의도 ① 진입점을 `wWinMain`으로 한 이유
- 콘솔 앱은 `main`, **GUI 앱은 `(w)WinMain`** 이 진입점이다(`.vcxproj`의 SubSystem=Windows와 짝).
- `w`가 붙은 와이드 버전을 쓴 건, 프로젝트를 **유니코드**로 설정했기 때문(한글 경로/제목 안전).

### 의도 ② Window와 Application을 분리한 이유
- 창은 "OS와 대화하는 창구", 앱은 "게임 흐름의 주인". 책임이 다르다.
- 이렇게 두면 나중에 멀티 윈도우, 또는 창 없이 도는 테스트 등 변화에 유연하다.

### 의도 ③ `PeekMessage` 루프를 쓴 이유 (가장 중요)
- 일반 윈도우 앱은 `GetMessage`로 **메시지가 올 때까지 멈춰(블로킹)** 기다린다 → 이벤트 구동.
- 하지만 게임은 **입력이 없어도 매 프레임 화면을 갱신**해야 한다(애니메이션, 물리 등).
- 그래서 **`PeekMessage`(메시지 없으면 즉시 반환=비블로킹)** 로 "쌓인 메시지만 처리하고
  곧바로 Update/Render로 넘어가는" 구조를 만들었다. **이것이 게임 루프의 핵심 차이다.**

### 의도 ④ WndProc-객체 연결에 "Setup/Thunk" 패턴을 쓴 이유
- Win32의 `WndProc`는 **C 함수 포인터**라서, C++ **멤버 함수**(숨은 `this` 인자 때문에)를
  직접 등록할 수 없다.
- 해결: 창을 만들 때 `this`를 넘겨두고, 첫 메시지에서 그 `this`를 `HWND`의 사용자 데이터
  슬롯에 저장 → 이후 모든 메시지를 멤버 함수 `HandleMessage`로 넘긴다.
- 이 패턴을 쓰면 메시지 처리 코드 안에서 `m_width` 같은 **멤버 변수에 자연스럽게 접근**할 수 있다.

---

## 4. 코드 깊이 읽기

### 4-1. 창 생성 (`Window::Create`)
```cpp
WNDCLASSEXW wc = {};
wc.lpfnWndProc   = &Window::WndProcSetup; // 최초 메시지를 받을 함수
wc.hInstance     = hInstance;
wc.lpszClassName = kClassName;
RegisterClassExW(&wc);                    // ① "창 설계도"를 OS에 등록
```
- `WNDCLASSEXW`는 **창의 종류(클래스)** 를 정의하는 설계도다. 아이콘/커서/배경/메시지 처리 함수 등.
- 실제 창을 만들기 전에 이 설계도를 **OS에 등록**해야 한다.

```cpp
RECT rect = { 0, 0, width, height };
AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE); // ② 테두리 보정
```
- `CreateWindow`에 주는 크기는 **테두리+타이틀바를 포함한 전체 크기**다.
- 우리가 원하는 건 **그림이 그려질 안쪽(클라이언트 영역)** 이 정확히 1280×720인 것.
- `AdjustWindowRect`가 "안쪽을 이 크기로 만들려면 바깥은 얼마여야 하는지"를 계산해준다.
- **왜 중요?** Phase 2에서 스왑체인 해상도를 클라이언트 크기에 맞추는데, 여기가 어긋나면
  렌더링이 늘어나거나 잘린다.

```cpp
m_hwnd = CreateWindowExW(..., hInstance, this); // ③ 마지막 인자에 this!
```
- 마지막 인자 `this`가 의도 ④의 출발점. 곧 `WM_NCCREATE`에서 회수한다.

### 4-2. 메시지 루프 (`Window::ProcessMessages`)
```cpp
MSG msg = {};
while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) return false; // 종료 신호
    TranslateMessage(&msg);  // 키 입력 → 문자 메시지 변환
    DispatchMessageW(&msg);  // 실제 WndProc(=Thunk) 호출
}
return true;
```
- `PM_REMOVE`: 본 메시지는 큐에서 제거한다.
- `WM_QUIT`을 만나면 `false`를 반환 → `Application::Run`의 루프가 끝난다.
- `DispatchMessage`가 우리 `WndProcThunk`를 호출하고, 그게 `HandleMessage`로 이어진다.

### 4-3. this 연결 (`WndProcSetup` / `WndProcThunk`)
```cpp
LRESULT Window::WndProcSetup(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_NCCREATE) {                         // 창 최초 메시지
        auto* create = (CREATESTRUCTW*)l;
        auto* self   = (Window*)create->lpCreateParams; // ③에서 넘긴 this 회수
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self); // HWND에 저장
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)&WndProcThunk); // 이후엔 Thunk
        return self->HandleMessage(hwnd, msg, w, l);
    }
    return DefWindowProcW(hwnd, msg, w, l);
}
```
- `WM_NCCREATE`는 창이 만들어질 때 **가장 먼저** 오는 메시지. 여기서 `this`를 회수해 저장한다.
- 저장 후 WndProc 자체를 `WndProcThunk`로 바꿔, 이후 메시지는 곧장 thunk가 받는다.
- `WndProcThunk`는 저장해둔 `this`를 꺼내 `HandleMessage`로 위임한다(아주 짧다).

### 4-4. 실제 메시지 처리 (`Window::HandleMessage`)
```cpp
case WM_CLOSE:   DestroyWindow(hwnd);  return 0; // X 버튼/Alt+F4
case WM_DESTROY: PostQuitMessage(0);   return 0; // 창 파괴 → 큐에 WM_QUIT
case WM_SIZE:    m_width=LOWORD(l); m_height=HIWORD(l); ... return 0; // 리사이즈
case WM_KEYDOWN: if (w==VK_ESCAPE) DestroyWindow(hwnd); return 0;     // ESC 종료
```
- **종료 흐름이 핵심**: `X`/`ESC` → `WM_CLOSE`/`DestroyWindow` → `WM_DESTROY` →
  `PostQuitMessage` → 큐에 `WM_QUIT` → `ProcessMessages`가 `false` → 루프 종료.
- `WM_SIZE`에서 클라이언트 크기를 멤버에 저장한다. **Phase 2에서 이 값으로 스왑체인을 리사이즈**한다.

### 4-5. 게임 루프 (`Application::Run`)
```cpp
while (m_window.ProcessMessages()) { // 메시지 처리(종료면 false)
    Update();  // 게임 상태 갱신 (지금은 프레임 카운트)
    Render();  // 화면 그리기 (Phase 2에서 D3D12가 채움)
}
```
- **모든 게임의 심장**: 입력 처리 → 업데이트 → 렌더를 반복.
- 지금 `Render()`는 비어 있다. Phase 2에서 이 한 줄 안이 D3D12 클리어+Present로 채워진다.

---

## 5. 여기서 배우는 지식 (로드맵 "선행 지식" 연결)
- **이벤트 구동 vs 게임 루프**: `GetMessage`(블로킹) ↔ `PeekMessage`(비블로킹)의 본질적 차이.
- **Win32 메시지 흐름**: `WM_NCCREATE → WM_CREATE → ... → WM_SIZE → ... → WM_DESTROY → WM_QUIT`.
- **콜백 ↔ C++ 객체 연결 패턴**: C API 위에 C++ 객체지향을 얹는 일반적 기법(엔진 곳곳에서 재등장).
- **클라이언트 영역 vs 윈도우 영역**: 렌더 해상도 계산의 기초.
- **유니코드(W) API**: Win32에서 `...W` 함수와 와이드 문자열을 쓰는 이유.

---

## 6. 완료 기준 / 다음 단계
**완료 기준** (`QUICKSTART.md`와 동일):
- [ ] 1280×720 창이 뜬다
- [ ] ESC 또는 X로 정상 종료된다
- [ ] 리사이즈 시 출력 창에 `리사이즈: WxH` 로그
- [ ] `동작 중... frame=N` 로그가 주기적으로 출력

**다음: Phase 2 — Direct3D 12 초기화 (화면 단색 클리어)**
`Application::Render()` 안에 D3D12 렌더 루프가 들어가고, `src/RHI/`에 디바이스/스왑체인/
커맨드 큐/펜스 래퍼가 추가된다. (로드맵의 "D3D12의 80%"라고 표시된 그 단계)
