# Phase 0 — 개발 환경 & 프로젝트 세팅

## 1. 이번 Phase의 목표
코드를 한 줄도 짜기 전에, **"빌드하고 디버깅할 수 있는 토대"** 를 갖추는 단계.
도구 확인 → 프로젝트 구조 결정 → 버전 관리(git) 등록까지가 목표다.

---

## 2. 무엇을 했나

### 2-1. 설치된 도구 점검
PC 상태를 확인한 결과:

| 도구 | 상태 | 비고 |
|------|------|------|
| Visual Studio 2022 Community | ✅ 설치됨 | IDE 본체 |
| MSBuild | ✅ 있음 | 빌드 엔진 |
| Git | ✅ 설치됨 | 2.53 |
| **MSVC 컴파일러 (cl.exe)** | ❌ **없음** | `VC\Tools\MSVC` 폴더 부재 |
| **Windows SDK** | ❌ **없음** | D3D12 헤더가 여기에 있음 |

→ 즉 **"C++를 사용한 데스크톱 개발" 워크로드가 빠진 상태**임을 발견.
이게 없으면 C++ 컴파일 자체가 불가능하므로, 가장 먼저 설치해야 하는 항목으로 안내했다.

#### 📖 보충: MSVC 컴파일러와 Windows SDK가 뭔가?
위 표에서 빠져 있던 두 가지가 가장 중요하다. 역할이 완전히 다르다.

> **MSVC = 코드를 실행파일로 "번역"하는 도구 (변환기)**
> **Windows SDK = Windows 기능을 "호출하는 법" — 헤더/라이브러리/도구 (재료)**

**① MSVC 컴파일러 (Microsoft Visual C++)**
Microsoft가 만든 C/C++ 툴체인. 우리가 쓴 `.cpp` 텍스트를 CPU가 실행할 수 있는 기계어(.exe)로
바꿔준다. 실제로는 여러 도구의 묶음이다.

| 도구 | 하는 일 |
|------|---------|
| `cl.exe` | 컴파일러. `.cpp` → `.obj`(기계어 조각) |
| `link.exe` | 링커. 여러 `.obj` + 라이브러리(`.lib`)를 묶어 하나의 `.exe`로 |
| 표준 C++ 라이브러리 | `std::vector`, `printf` 등 표준 기능의 구현체 |

- "MSVC가 없다" = `cl.exe`가 없다 = **C++ 코드를 .exe로 만들 방법이 아예 없다.**
  Phase 0에서 빌드가 불가능했던 직접적인 이유다.
- `.vcxproj`의 `PlatformToolset = v143`이 바로 **VS2022용 MSVC 툴셋 버전**을 가리킨다.
- 경쟁 컴파일러: GCC(리눅스), Clang(맥/크로스플랫폼). Windows의 DirectX 개발은 MSVC가 표준.

**② Windows SDK (Software Development Kit)**
내 프로그램은 직접 창을 띄우거나 GPU를 다룰 수 없다. 그 일은 전부 **Windows(OS)** 가 한다.
그래서 "Windows야, 창 만들어줘"라고 **함수로 부탁**해야 하는데, 그 부탁하는 규격을 모아둔 것이 SDK다.

| 구성 | 예시 | 역할 |
|------|------|------|
| 헤더(.h) | `Windows.h`, `d3d12.h`, `dxgi1_6.h` | "이런 함수/구조체가 있다"는 **선언(설명서)** |
| 라이브러리(.lib) | `user32.lib`, `d3d12.lib` | 그 함수와 OS의 실제 구현(.dll)을 잇는 **연결 고리** |
| 도구 | `dxc.exe` | HLSL 셰이더 컴파일러 등 SDK에 딸려오는 개발 도구 |

이미 쓴 코드로 보면:
```cpp
#include <Windows.h>      // ← Windows SDK 헤더
CreateWindowExW(...);     // ← 선언은 Windows.h, 실제 구현은 OS의 user32.dll,
                          //    둘을 잇는 게 user32.lib
```
그리고 **가장 중요한 점**: Direct3D 12도 SDK의 일부다(별도 설치 아님).
```cpp
#include <d3d12.h>        // ← D3D12 헤더 = Windows SDK에 포함
D3D12CreateDevice(...);   // ← GPU 디바이스 생성 요청 (Phase 2에서 사용)
```

**③ 둘의 관계 (비유)** — 집(.exe)을 짓는다면:
- **MSVC** = 벽돌을 쌓아 집을 짓는 **공사 장비/인부** (조립하는 쪽)
- **Windows SDK** = 창문·콘센트 규격과 배관 연결구가 적힌 **표준 부품 카탈로그** (재료/규격)

→ 변환기(MSVC) + 재료(SDK), **둘 다 있어야** 빌드가 완성된다.
"C++를 사용한 데스크톱 개발" 워크로드 하나가 이 둘을 **함께** 설치해준다.
(컴파일·링크의 전체 흐름은 → [`docs/concepts/01_Compile_and_Build.md`](concepts/01_Compile_and_Build.md))

### 2-2. 프로젝트 폴더 구조 생성
```
DKEngine/
├─ DKEngine.sln / .vcxproj / .filters   ← Visual Studio 프로젝트
├─ .gitignore
├─ ROADMAP.md        ← 전체 학습 로드맵
├─ QUICKSTART.md     ← 설치·빌드·실행 가이드
├─ docs/             ← 단계별 개발 노트(이 폴더)
└─ src/
   ├─ main.cpp
   └─ Core/          ← 엔진 코어(창, 루프, 로그)
```

### 2-3. 빌드 시스템 선택: Visual Studio 솔루션(.sln/.vcxproj)
CMake가 아니라 **VS 네이티브 프로젝트**를 선택했다.

#### 📖 보충: 빌드 시스템이란? VS 솔루션 vs CMake
소스 파일이 몇 개를 넘어가면 "어떤 파일을 / 어떤 옵션으로 / 어떤 순서로 빌드할지"를 사람이 매번
손으로 지정하기 어렵다. 이 **빌드 절차를 파일로 적어두고 자동화하는 도구**가 빌드 시스템이다.

**① Visual Studio 솔루션 (.sln / .vcxproj)** — 우리가 선택한 방식
- **`.vcxproj` (프로젝트)**: 빌드 한 단위. "이 소스들을 이 옵션(C++17, x64, Unicode...)으로
  빌드해 `DKEngine.exe`를 만들어라"가 적힌 XML 파일. 사실 **MSBuild가 읽는 빌드 스크립트**다.
- **`.sln` (솔루션)**: 여러 프로젝트를 묶는 상위 컨테이너. 지금은 프로젝트가 하나뿐이지만,
  나중에 `Engine`(라이브러리)과 `Game`(실행파일)으로 나누면 한 솔루션 아래 두 프로젝트가 된다.
- **`.vcxproj.filters`**: 솔루션 탐색기에서 보이는 가상 폴더(분류)만 정의. 실제 디스크 폴더와 무관.
- 즉 VS에서 F5를 누르면 → VS가 `.sln`/`.vcxproj`를 읽어 → **MSBuild + MSVC**를 호출해 빌드한다.
- **장점**: Windows/VS에 딱 붙어 있어 디버거·IntelliSense·PIX 연동이 즉시 된다(설정 0).
- **단점**: Windows + Visual Studio 전용. 맥/리눅스나 다른 IDE에서는 그대로 못 쓴다.

**② CMake** — 우리가 **지금은** 안 쓴 방식
- 빌드 절차를 `CMakeLists.txt`라는 **플랫폼 중립 스크립트**로 한 번만 적는다.
- 그러면 CMake가 그것을 읽어 각 환경에 맞는 실제 빌드 파일을 **생성(generate)** 해준다:
  Windows에서는 `.vcxproj`(또는 Ninja), 맥에서는 Xcode, 리눅스에서는 Makefile 등.
- 비유: CMake는 **"빌드 설명서를 각 나라 언어로 번역해주는 번역기"**. 한 번 쓰면 어디서든 빌드 가능.
- **장점**: 크로스플랫폼, 오픈소스 라이브러리 대부분이 CMake를 씀(가져다 쓰기 편함), CI 자동화에 유리.
- **단점**: 문법을 따로 배워야 하고, "내 코드 → CMake → vcxproj → MSVC"로 **한 겹이 더** 끼어
  초보 단계에선 문제 원인 추적이 어려워진다.

**왜 지금은 VS 솔루션인가** (요지): 학습 초기엔 **디버깅 경험의 매끄러움**이 더 중요하고,
우리는 Windows 전용 D3D12를 만들므로 크로스플랫폼 이점이 당장 필요 없다.
크로스플랫폼/CI가 필요해지는 시점(로드맵 Phase 4 이후)에 CMake로 전환하면 된다.

### 2-4. git 초기화 & 원격 연결
`git init` → 첫 커밋 → `origin`(GitHub) 연결 → `push` 까지 완료.

---

## 3. 왜 이렇게 했나 (의도)

### 의도 ① "C++를 사용한 데스크톱 개발" 워크로드를 권장한 이유
- 우리는 **D3D12를 바닥부터 직접** 만든다. 필요한 건 결국 **컴파일러 + Windows SDK** 둘뿐이다.
- **D3D12/DXGI 헤더(`d3d12.h`, `dxgi1_6.h`)와 HLSL 컴파일러(dxc)는 Windows SDK 안에** 들어 있고,
  그 SDK는 데스크톱 개발 워크로드에 포함된다. 그래서 이 워크로드만으로 충분하다.
- "게임 개발 C++" 워크로드는 Unreal/Cocos 같은 **상용 엔진 연동용**이라 자작 엔진엔 과하다.

### 의도 ② 빌드 시스템을 CMake가 아닌 VS 솔루션으로 한 이유
- 로드맵의 원칙: **"디버거가 학습의 절반"**, 그리고 **PIX(GPU 디버거) 연동**.
  VS 프로젝트는 F5 한 번으로 중단점 디버깅·GPU 캡처가 매끄럽게 된다.
- CMake는 이식성은 좋지만 초기 학습 단계에선 한 겹의 추상화가 더 끼어 "왜 안 되지?"의 원인이 늘어난다.
- 트레이드오프: 나중에 크로스플랫폼/CI가 필요해지면 CMake로 전환 가능(로드맵 Phase 4 이후 선택지).

### 의도 ③ `src/Core/` 처럼 모듈을 미리 나눈 이유
- Phase가 진행되면 `RHI/`(D3D12 추상화), `Renderer/`, `Math/`, `Scene/` 가 추가된다.
- 처음부터 **관심사별 폴더**를 잡아두면, 나중에 파일이 늘어도 구조가 무너지지 않는다.
- `Core`는 "어느 게임에나 필요한 가장 바깥 뼈대"(앱, 창, 시간, 로그)를 담는 자리다.

### 의도 ④ git을 일찍 등록한 이유
- 로드맵 운영 원칙: **"각 Phase = 학습 → 구현 → 검증 → 커밋"**.
- D3D12는 한 줄 잘못 바꾸면 화면이 까맣게 죽는 일이 잦다. **"동작하던 마지막 커밋"** 이 있으면
  언제든 안전하게 되돌아갈 수 있다. 그래서 코드가 적을 때 미리 안전망을 깔았다.

---

## 4. `.vcxproj` 핵심 설정 해설
`DKEngine.vcxproj`에서 의미 있는 설정들:

| 설정 | 값 | 의미 |
|------|----|------|
| `ConfigurationType` | `Application` | 실행 파일(.exe) 생성 |
| `PlatformToolset` | `v143` | VS2022의 MSVC 툴셋 |
| `WindowsTargetPlatformVersion` | `10.0` | 설치된 **최신** Windows SDK 자동 선택 |
| `LanguageStandard` | `stdcpp17` | C++17 |
| `CharacterSet` | `Unicode` | Win32 API를 `...W`(와이드) 버전으로 사용 |
| `SubSystem` | `Windows` | GUI 앱 → 진입점이 `wWinMain` |
| `OutDir/IntDir` | `build\...` | 산출물을 `build/`에 모아 소스와 분리(.gitignore 처리) |

- **`x64` 단독 구성**: D3D12는 사실상 64비트가 표준이라 32비트(Win32) 구성은 만들지 않았다.
- **Debug/Release 분리**: Debug는 최적화 끔+디버그 정보, Release는 최적화 켬. 성능 측정은 항상 Release로.

---

## 5. 여기서 배우는 지식 (로드맵 연결)
- **워크로드 ↔ SDK ↔ 헤더의 관계**: "D3D12를 쓴다"는 건 결국 Windows SDK의 헤더/lib를 링크하는 것.
- **빌드 구성(Configuration/Platform)** 개념: 같은 소스가 Debug/Release·x64로 다르게 빌드된다.
- **버전 관리 습관**: 작동 기준점을 커밋으로 남기는 것이 그래픽스 디버깅에서 특히 강력하다.

---

## 6. 완료 기준 / 다음 단계
- [x] 프로젝트 구조 생성
- [x] VS 솔루션/프로젝트 작성
- [x] git init + 첫 커밋 + 원격 push
- [ ] **"C++를 사용한 데스크톱 개발" 워크로드 설치** ← 빌드하려면 이게 남아 있음

다음: **Phase 1 빌드 확인** → [Phase01_Win32Window.md](Phase01_Win32Window.md)
