# C++ 클래스와 객체 모델 (this · new · 값 vs 포인터)

C#/Java를 하다 C++에 오면 거의 반드시 걸리는 두 가지 질문을 정리한다.

> Q1. 멤버 함수는 어떻게 매개변수로 안 받은 `m_width`를 바로 쓰지? → **숨은 `this` 포인터**
> Q2. 클래스는 무조건 `new` 해야 하는 거 아냐? → **C++에선 `new`가 필수가 아님**

핵심 한 줄:
> **C++은 "타입이 무엇인가"와 "객체가 어디 사는가(스택/힙)"를 분리한다.**
> C#은 `class`면 무조건 참조·힙·`new`지만, C++은 **선언 방식**이 위치를 정한다.

---

## 1. 클래스는 "설계도(타입)"지, 전역 객체가 아니다

```cpp
class Window { int m_width; bool Create(...); };  // ← 타입 정의. 메모리 없음(붕어빵 틀)
```
- `#include "Window.h"`는 "이 타입의 생김새를 알려줘"일 뿐, 전역 객체를 만드는 게 아니다.
- 실체(객체)는 따로 만들어야 메모리가 생긴다: `Window w;` (붕어빵).

우리 프로젝트의 소유 관계(전역이 아님):
```cpp
// main.cpp
dk::Application app;          // 스택 지역변수
// Application.h
class Application { Window m_window; };  // app 안에 m_window가 통째로 들어있음
```

---

## 2. Q1 — 숨은 `this` 포인터

이런 코드가 가능한 이유:
```cpp
LRESULT Window::HandleMessage(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    case WM_SIZE:
        m_width = LOWORD(l);   // m_width를 안 넘겼는데 어떻게?
```

**멤버 함수는 "어떤 객체에 대해" 호출되며, 그 객체의 주소가 `this`로 몰래 전달된다.**
컴파일러가 속으로 하는 일:
```cpp
LRESULT Window::HandleMessage(Window* this, HWND hwnd, ...) {
    this->m_width = LOWORD(l);   // m_width == this->m_width
}
```
`m_width`라고만 써도 컴파일러가 `this->m_width`로 해석한다. 즉 **그 객체가 가진 변수**에 접근.

### C로 비유 (가장 명확)
```c
// C 스타일 — self를 직접 넘김
typedef struct { int width; } Window;
bool Window_Create(Window* self, ...) { self->width = 1280; }
Window w;  Window_Create(&w, ...);
```
```cpp
// C++ — self를 this로 자동화한 것뿐
bool Window::Create(...) { m_width = 1280; }  // this(&w)가 숨어서 들어옴
Window w;  w.Create(...);   // 컴파일러가 Window::Create(&w, ...) 로 변환
```
→ `w.Create(...)`의 점(`.`) 왼쪽 `w`가 곧 `this`. 그래서 매개변수로 안 받아도 멤버를 쓴다.

### 매개변수 vs 멤버 변수
| 종류 | 예 | 성격 |
|------|----|------|
| 매개변수 | `hwnd`, `msg`, `lParam` | 호출 때마다 외부에서 전달(이번 입력) |
| 멤버 변수 | `m_width`, `m_hwnd` | 객체가 지속 보관하는 상태(`this`로 접근) |

`WM_SIZE`에서 `m_width = ...` 하면 객체에 **저장**되어, 나중에 `Width()`로 꺼내 쓸 수 있다(Phase 2 스왑체인 리사이즈에 사용).

---

## 3. Q2 — `new`는 필수가 아니다

### C#의 사고방식 (타입이 값/참조를 결정)
```csharp
class  Window { } // 참조 타입 → 힙 → 무조건 new
struct Point  { } // 값 타입   → 인라인
Window w = new Window();
```
C#은 **`class` = 참조 = 힙 = `new` 필수**가 한 묶음.

### C++ (선언 방식이 위치를 결정)
```cpp
Window a;                              // ① 스택/인라인. new 없음!
Window* b = new Window();              // ② 힙. 나중에 delete b; 필요
auto    c = std::make_unique<Window>();// ③ 힙이지만 자동 관리
```
셋 다 같은 `Window` 타입. 차이는 **위치 / 수명 관리 주체**뿐.

| 방식 | 위치 | 생성 | 소멸 | new |
|------|------|------|------|-----|
| `Window a;` | 스택/인라인 | 자동(생성자) | **자동**(스코프 끝, 소멸자) | ❌ |
| `new Window()` | 힙 | 명시 `new` | **수동** `delete` | ✅ |
| `make_unique` | 힙 | 스마트 포인터 | 자동(참조 끝) | (내부) |

### 우리 코드의 메모리 그림
```cpp
dk::Application app;       // new 없음 (스택)
class Application { Window m_window; };  // app 안에 인라인 포함
```
```
[ wWinMain 스택 ]
   app (Application)
      └─ m_window (Window)   ← app 안에 통째로 박힘. 별도 힙 할당 X
            ├─ m_hwnd / m_width / ...
```
`app` 생성 시 `m_window`도 함께 생성, `app` 소멸 시 함께 소멸. **new도 delete도 불필요.**

---

## 4. "Window는 값 타입인가?"

엄밀히는 **C++에 C#식 "값 타입/참조 타입" 고정 분류가 없다.** 어떤 타입이든
- 값처럼(`Window a;`) 쓸 수도, 포인터로(`new Window`) 쓸 수도 있다.

→ 우리는 `Window m_window;`로 **"값처럼(by value)" 사용**할 뿐, 타입이 값 타입이어서가 아니다.

> **`class` vs `struct` 차이**: C++에선 **기본 접근 제한만 다르다**(class=private, struct=public).
> C#처럼 class=참조 / struct=값 같은 의미가 **전혀 없다.**

C# ↔ C++ 매핑:
| C# | C++ 대응 |
|----|---------|
| C# `class` (참조·힙·GC) | `Window* p = new Window;` 또는 `unique_ptr<Window>` |
| C# `struct` (값·인라인) | `Window w;` (지금 우리 방식) |

---

## 5. C++에서 `new`는 언제 쓰나
스택/값으로 안 되고 **힙이 꼭 필요할 때만**:
- 객체가 현재 스코프보다 **오래 살아야** 할 때
- 개수/크기가 **런타임에 결정**될 때
- **다형성**: `Base* p = new Derived;`
- 객체가 너무 커서 스택에 두기 부담될 때

그리고 현대 C++(C++11~)은 raw `new`/`delete` 대신 **`std::unique_ptr`/`std::make_unique`** 사용
(자동 `delete` → 누수 방지). → 로드맵 Phase 4의 RAII·스마트 포인터 주제.

**우리가 값으로 둔 이유**: 가장 단순·안전. new/delete가 없으니 누수 가능성 자체가 없고,
수명이 소유자(Application)에 자동으로 묶인다. **"기본은 값/스택, 힙은 꼭 필요할 때만"** 이 권장 스타일.

---

## 6. 한 줄 요약
- 클래스는 **타입(설계도)**. `#include`는 생김새만 알려줌(전역 객체 아님).
- 멤버 함수엔 **숨은 `this`** 가 있어 `m_width` == `this->m_width`. (C의 `self`를 자동화한 것)
- C++에서 `new`는 **필수가 아니라 "힙에 만드는 한 방법"**. C#/Java만 class에 강제.
- `Window m_window;`는 new 없이 **값으로** 만든 것 → 자동 생성·자동 소멸.
- C++엔 값/참조 타입 고정 분류 없음. `class`/`struct`는 기본 접근 제한만 차이.

> 관련: 컴파일/링크는 [`01_Compile_and_Build.md`](01_Compile_and_Build.md),
> 코드 전체 맥락은 [`../Phase01_Win32Window.md`](../Phase01_Win32Window.md) 참고.
