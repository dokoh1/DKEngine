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
