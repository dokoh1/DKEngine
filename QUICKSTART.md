# DKEngine 빠른 시작 (Phase 2)

## 0. 먼저: C++ 빌드 도구 확인

현재 PC에서는 Debug/x64 빌드가 확인된 상태입니다.

- MSBuild: 17.14
- MSVC: 14.44.35207
- Windows SDK: 10.0.26100.0
- `d3d12.h`: 설치 확인됨
- PIX: `C:\Program Files\Microsoft PIX` 설치 확인됨

즉 지금은 바로 빌드/실행을 시도하면 됩니다. 아래 설치 절차는 다른 PC에서 열었거나,
`cl.exe`, Windows SDK, `d3d12.h`를 찾지 못할 때만 진행합니다.

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

설치 후 `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\` 폴더와
`C:\Program Files (x86)\Windows Kits\10\Include\...\um\d3d12.h`가 있으면 준비 완료입니다.

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

현재 확인 결과: `Debug|x64`, `Release|x64` 빌드 성공, 경고 0개 / 오류 0개.

PIX 캡처 방법은 [`docs/PIX_Usage.md`](docs/PIX_Usage.md)를 참고합니다.

---

## 2. Phase 2 완료 기준 체크
- [x] 창이 1280x720으로 뜬다
- [x] D3D12 초기화가 성공한다
- [x] 창 전체가 지정한 색으로 clear/present 된다
- [x] 실행 후 정상 종료 코드 0을 반환한다
- [x] `Debug|x64`, `Release|x64` 빌드가 경고 0개 / 오류 0개로 통과한다

리사이즈 시 `D3D12 백버퍼 리사이즈: WxH` 로그가 찍히면 스왑체인 리사이즈 흐름도 정상입니다.

---

## 3. 다음 단계 (Phase 3)
셰이더, 루트 시그니처, PSO, 정점 버퍼를 추가해 **첫 삼각형**을 그립니다.

## 코드 구조 (현재)
```
DKEngine/
├─ DKEngine.sln / DKEngine.vcxproj   ← Visual Studio 프로젝트
├─ src/
│  ├─ main.cpp                       ← wWinMain 진입점
│  ├─ Core/
│     ├─ Application.{h,cpp}         ← 게임 루프 (Update/Render)
│     ├─ Window.{h,cpp}             ← Win32 윈도우 + 메시지 처리
│     ├─ Log.h                       ← OutputDebugString + Debug 콘솔 로깅
│     └─ WindowsMinimal.h            ← Windows.h 포함 정책
│  └─ RHI/
│     └─ D3D12Renderer.{h,cpp}       ← D3D12 화면 클리어 렌더러
├─ ROADMAP.md                        ← 전체 학습 로드맵
└─ QUICKSTART.md                     ← 이 문서
```
