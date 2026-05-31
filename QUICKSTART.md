# DKEngine 빠른 시작 (Phase 1)

## 0. 먼저: C++ 빌드 도구 설치 (필수, 1회만)

현재 PC에는 Visual Studio 2022 Community는 있지만 **"C++를 사용한 데스크톱 개발" 워크로드가
설치돼 있지 않아** 빌드가 불가능합니다. (MSVC 컴파일러 + Windows SDK가 이 워크로드에 포함됩니다.
Windows SDK 안에 Phase 2부터 쓸 D3D12 헤더도 들어 있습니다.)

### 방법 A — GUI (권장, 가장 확실)
1. 시작 메뉴에서 **"Visual Studio Installer"** 실행
2. DKEngine(VS 2022 Community) 항목에서 **수정(Modify)** 클릭
3. **"C++를 사용한 데스크톱 개발"** 워크로드 체크
4. 오른쪽 "설치 세부 정보"에 **최신 Windows 11 SDK**가 포함됐는지 확인 후 **수정/설치**

### 방법 B — 명령줄 (관리자 권한 PowerShell)
채팅 입력창에 아래를 `!` 를 붙여 실행하거나, 관리자 PowerShell에서 직접 실행:

```
& "C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe" modify `
  --installPath "C:\Program Files\Microsoft Visual Studio\2022\Community" `
  --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended
```

설치 후 `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\` 폴더가
생기면 준비 완료입니다.

---

## 1. 빌드 & 실행

### 방법 A — Visual Studio (권장, 디버깅/PIX 연동 용이)
1. `DKEngine.sln` 더블클릭
2. 상단 구성에서 **Debug / x64** 선택
3. **F5** (디버깅 시작) → 1280x720 창이 뜸

### 방법 B — 명령줄 빌드
개발자 PowerShell(또는 vcvars64 적용된 셸)에서:
```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  DKEngine.sln /p:Configuration=Debug /p:Platform=x64
```
산출물: `build\x64\Debug\DKEngine.exe`

---

## 2. Phase 1 완료 기준 체크
- [ ] 창이 1280x720으로 뜬다
- [ ] **ESC** 또는 창 **X** 버튼으로 정상 종료된다
- [ ] 창 크기를 바꾸면 VS **출력(Output)** 창에 `리사이즈: WxH` 로그가 찍힌다
- [ ] 출력 창에 `동작 중... frame=N` 로그가 주기적으로 찍힌다

이 4가지가 확인되면 Phase 1 완료입니다. (로드맵 `ROADMAP.md` 참고)

---

## 3. 다음 단계 (Phase 2)
화면을 매 프레임 단색으로 지우는 **D3D12 초기화**로 넘어갑니다.
`Application::Render()` 자리에 D3D12 렌더러가 들어가며, `src/RHI/` 모듈이 추가됩니다.

## 코드 구조 (현재)
```
DKEngine/
├─ DKEngine.sln / DKEngine.vcxproj   ← Visual Studio 프로젝트
├─ src/
│  ├─ main.cpp                       ← wWinMain 진입점
│  └─ Core/
│     ├─ Application.{h,cpp}         ← 게임 루프 (Update/Render)
│     ├─ Window.{h,cpp}             ← Win32 윈도우 + 메시지 처리
│     └─ Log.h                       ← OutputDebugString 기반 로깅
├─ ROADMAP.md                        ← 전체 학습 로드맵
└─ QUICKSTART.md                     ← 이 문서
```
