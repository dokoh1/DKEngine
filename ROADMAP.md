# DKEngine — Direct3D 12 기반 자작 엔진 & 2D 게임 개발 로드맵

## 이 문서의 목적

C/C++/C#에 어느 정도 익숙하고 렌더링 파이프라인의 기초 지식이 있는 상태에서,
**Direct3D 12를 바닥부터 직접 다루며** 자작 엔진(DKEngine)을 만들고, 최종적으로 그 엔진으로
**2D 게임을 완성**하는 것을 목표로 한다. 단순히 "동작하는 엔진"을 만드는 것이 목적이 아니라,
**각 기능을 구현하기 위해 어떤 DirectX/CS/수학 지식이 선행되어야 하는지를 함께 학습**하는 것이
핵심이다.

각 Phase는
**[목표] · [구현할 기능] · [선행 지식(DirectX)] · [선행 지식(CS·수학)] · [추천 자료] · [완료 기준]**
구조로 되어 있어, "무엇을 만들고, 만들기 위해 무엇을 알아야 하는지"를 동시에 따라갈 수 있다.

---

## 0. 큰 그림 — 왜 D3D12는 어렵고, 어떻게 접근할 것인가

Direct3D 12는 D3D11과 달리 **드라이버가 자동으로 해주던 일을 개발자가 명시적으로** 한다.
즉 다음을 직접 관리해야 한다:

- **GPU 메모리 할당과 CPU↔GPU 동기화** (Fence)
- **커맨드 기록과 제출** (Command List / Command Queue)
- **리소스 상태 전이** (Resource Barrier)
- **디스크립터(리소스 핸들) 관리** (Descriptor Heap)

이 "명시성" 때문에 첫 삼각형까지 도달하는 데만 D3D11보다 훨씬 많은 코드가 필요하다.
따라서 이 로드맵은 **작은 성공을 빠르게 쌓는 순서**(윈도우 → 화면 클리어 → 삼각형 → 텍스처 → 스프라이트)로
설계했고, 각 단계에서 막히지 않도록 선행 지식을 앞에 배치했다.

### 전제 도구 (Phase 0에서 설치/확인)
- **Visual Studio 2022** (C++ 데스크톱 개발 워크로드) — 디버거가 학습의 절반이다.
- **Windows 10/11 SDK** (D3D12 헤더, dxc, PIX 연동 포함)
- **PIX on Windows** (GPU 캡처/디버깅 — D3D12 학습 필수 도구)
- 선택: **CMake** (장기적으로 빌드 이식성), **Git** (버전 관리, 단계별 커밋 권장)

### 관통하는 핵심 자료 (전 단계 공통)
- 📕 **Frank Luna, *Introduction to 3D Game Programming with DirectX 12*** — D3D12 입문 표준서
- 🌐 **3dgep.com — "Learning DirectX 12" 시리즈** (Jeremiah van Oosten) — 초기화/동기화 설명 탁월
- 🌐 **Microsoft `DirectX-Graphics-Samples`** (GitHub) — `D3D12HelloWorld` 부터 정독
- 🌐 **learn.microsoft.com — Direct3D 12 programming guide** — 개념 레퍼런스
- 📗 **Jason Gregory, *Game Engine Architecture*** — 엔진 구조 설계 (렌더링 외 시스템)

### 현재 아키텍처 원칙
- `Application`은 최상위 조율자다. 창, 렌더러, 나중의 입력/씬을 소유하되 D3D12 COM 객체를 직접
  들고 있지 않는다.
- Win32 세부 구현은 `Core/Window`에 가둔다. `HWND`는 렌더러 초기화에 필요한 핸들로만 노출한다.
- Phase 2부터 D3D12 코드는 `src/RHI/`에 둔다. 처음에는 작은 `D3D12Renderer`/`D3D12Context`로 시작하고,
  Phase 4에서 `Device`, `SwapChain`, `CommandQueue` 등으로 세분화한다.
- 엔진 코드는 기본적으로 RAII를 따른다. OS 핸들, COM 객체, GPU 리소스는 생성한 객체가 정리 책임도 갖는다.

---

## Phase 1 — Win32 윈도우 & 메시지 루프

**목표**: 운영체제 위에 창을 띄우고 게임 루프의 뼈대를 만든다. (아직 D3D 없음)

**구현할 기능**
- Win32 윈도우 생성 (`WNDCLASSEX`, `CreateWindowEx`)
- 메시지 펌프 (`PeekMessage` 기반 — 게임용 비블로킹 루프)
- `WndProc` 콜백, 윈도우 종료/리사이즈 메시지 처리
- 기본 `Application`/`Window` 클래스 분리

**선행 지식 — Windows/시스템**
- Win32 메시지 큐와 이벤트 구동 모델 (왜 `GetMessage` 대신 `PeekMessage`인가)
- `HINSTANCE`, `HWND`, 윈도우 메시지(`WM_SIZE`, `WM_DESTROY`, `WM_CLOSE`) 흐름
- 콜백 함수 포인터 / `static` 멤버를 통한 C++ 객체 ↔ WndProc 연결 패턴

**선행 지식 — CS**
- 이벤트 기반 프로그래밍 vs 폴링, 프로세스/스레드 기초

**추천 자료**: Luna 책 부록 A(Win32 기초), learn.microsoft.com "Your First Windows Program"

**완료 기준**: 창이 뜨고, ESC 또는 X로 정상 종료되며, 리사이즈 이벤트가 로그로 찍힌다.

---

## Phase 2 — Direct3D 12 초기화 (화면 클리어까지)

**목표**: D3D12 디바이스를 만들고 백버퍼를 매 프레임 단색으로 지운 뒤 Present 한다. **이 단계가 D3D12의 80%다.**

**구현할 기능**
0. `src/RHI/` 모듈 추가. `Application`은 렌더러를 호출만 하고 D3D12 세부 객체는 RHI가 소유
1. 디버그 레이어 활성화 (`D3D12GetDebugInterface`)
2. DXGI 팩토리 (`CreateDXGIFactory2`), 하드웨어 어댑터 열거/선택
3. 디바이스 생성 (`D3D12CreateDevice`, Feature Level 확인)
4. 커맨드 큐 (`ID3D12CommandQueue`, `DIRECT` 타입)
5. 스왑체인 (`IDXGISwapChain3`, **플립 모델** `FLIP_DISCARD`, 더블/트리플 버퍼링)
6. RTV 디스크립터 힙 + 백버퍼별 RTV 생성
7. 커맨드 할당자(`CommandAllocator`) + 커맨드 리스트(`GraphicsCommandList`)
8. **Fence 기반 CPU↔GPU 동기화** (`ID3D12Fence`, fence event)
9. 렌더 루프: 리소스 배리어(Present→RenderTarget) → ClearRenderTargetView → 배리어(RenderTarget→Present) → Execute → Present → Wait

**선행 지식 — DirectX(핵심)**
- **COM 모델**: `IUnknown`, 참조 카운팅, `Microsoft::WRL::ComPtr`, `HRESULT`/`FAILED()` 검사
- **GPU는 비동기 장치**다 → CPU가 기록(record)하고 큐에 제출(submit)하면 GPU가 나중에 실행
- **Fence**의 의미: GPU가 특정 지점까지 처리 완료했는지 CPU가 기다리는 신호값
- **디스크립터/디스크립터 힙**: 리소스 자체가 아니라 "리소스를 가리키는 핸들"의 배열
- **리소스 상태와 배리어**: 같은 메모리도 용도(렌더타겟/표시/읽기)에 따라 상태 전이 필요
- **스왑체인과 플립 모델**: 백버퍼/프론트버퍼, 더블 vs 트리플 버퍼링, V-Sync

**선행 지식 — CS**
- CPU/GPU 병렬성, 더블 버퍼링이 화면 찢김(tearing)을 막는 원리
- 메모리 정렬(alignment), 참조 카운팅 기반 수명 관리

**추천 자료**: 3dgep.com "Learning DirectX 12 — Part 1~2", Luna 4장, `D3D12HelloWorld/HelloFrameBuffering`

**완료 기준**: 창 전체가 매 프레임 지정한 색으로 칠해지고, 디버그 레이어 경고가 0이며,
종료 시 GPU 작업을 모두 기다린 후(`Flush`) 깨끗하게 정리된다.

---

## Phase 3 — 첫 삼각형 (그래픽스 파이프라인 직접 세팅)

**목표**: 정점 데이터를 GPU에 올리고 셰이더로 삼각형을 그린다. 프로그래머블 파이프라인을 손으로 조립한다.

**구현할 기능**
- HLSL 정점/픽셀 셰이더 작성 및 컴파일 (`dxc`/`D3DCompileFromFile`)
- **루트 시그니처**(`ID3D12RootSignature`) — 셰이더가 받을 리소스 레이아웃 정의
- **입력 레이아웃**(`D3D12_INPUT_ELEMENT_DESC`)
- **PSO(Pipeline State Object)** — 셰이더+블렌드+래스터라이저+뎁스+RTV 포맷을 한 객체로 고정
- 정점 버퍼: Upload 힙(간단) → 이후 Default 힙 + Upload 스테이징(정석)으로 발전
- 뷰포트/시저 설정, Draw 호출

**선행 지식 — DirectX(핵심)**
- **렌더링 파이프라인 단계**: IA → VS → (옵션 단계) → RS → PS → OM (각 단계가 무엇을 하는지)
- **PSO가 왜 존재하나**: D3D11의 상태 set 호출들을 하나로 묶어 드라이버 검증 비용을 앞당김
- **루트 시그니처 vs 디스크립터 테이블 vs 루트 상수**: 셰이더에 데이터를 넘기는 3가지 경로
- **Upload 힙 vs Default 힙**: CPU 접근 가능 메모리 vs GPU 전용 고속 메모리, 그리고 복사 패턴
- **HLSL 기초**: 시맨틱(`POSITION`, `SV_POSITION`), 상수 버퍼(`cbuffer`), register 바인딩

**선행 지식 — 수학**
- **정규화 장치 좌표(NDC)**: D3D는 x,y ∈ [-1,1], z ∈ [0,1]
- 정점/삼각형/와인딩 오더(front/back face), 컬링

**추천 자료**: Luna 5~6장, `D3D12HelloTriangle`, 3dgep "Part 3"

**완료 기준**: 컬러 삼각형이 화면에 뜨고, 정점 색이 보간되며, 셰이더 수정→재실행이 가능하다.

---

## Phase 4 — 렌더링 추상화 계층 정리

**목표**: Phase 2~3에서 만든 작은 RHI/렌더러를 재사용 가능한 클래스로 세분화한다. 이후 모든 단계의 토대.

**구현할 기능**
- `Device` / `SwapChain` / `CommandQueue` / `DescriptorAllocator` 래퍼 클래스
- `GpuBuffer`, `Texture`, `Shader`, `PipelineState`, `RootSignature` 추상화
- **프레임 리소스(Frame Resources)** 패턴 — 프레임마다 커맨드 할당자/상수버퍼 세트를 두어 GPU와 겹치지 않게
- RAII 기반 GPU 리소스 수명 관리, 에러/디버그 네임 부착

**선행 지식 — DirectX**
- **CPU-GPU 파이프라이닝**: GPU가 N프레임 늦게 실행되므로 N세트의 가변 리소스가 필요한 이유
- 디스크립터 힙을 어떻게 "할당기"로 추상화할지 (선형 할당 vs 링버퍼)

**선행 지식 — CS/C++**
- RAII, 이동 시맨틱(`std::unique_ptr`/move), 핸들 vs 포인터, 객체 수명과 GPU 비동기성의 충돌
- 인터페이스 설계 / 의존성 분리 (엔진 모듈화)

**추천 자료**: Gregory 책 "Engine Support Systems", 3dgep 후반부, MiniEngine(샘플) 구조 참고

**완료 기준**: Phase 3 삼각형을 정리된 RHI API만으로 다시 그릴 수 있고, `Application`에는 렌더링 세부 구현이 남지 않는다.

---

## Phase 5 — 텍스처 & 리소스 업로드

**목표**: 이미지를 GPU 텍스처로 올리고 셰이더에서 샘플링한다.

**구현할 기능**
- 이미지 로딩 (간단히 `stb_image.h` 사용 — 이미지 파싱은 학습 목표 밖)
- 텍스처 리소스 생성(Default 힙) + Upload 힙 스테이징 복사
- **SRV(Shader Resource View)** 를 CBV_SRV_UAV 힙에 생성, 샘플러 설정
- 셰이더에서 `Texture2D` + `SamplerState`로 샘플링
- 텍스처가 입혀진 사각형(Quad) 출력

**선행 지식 — DirectX**
- 텍스처 메모리 레이아웃, **row pitch / 정렬(256바이트)**, `UpdateSubresources`
- SRV / Sampler / 디스크립터 테이블 바인딩 흐름
- 밉맵, 텍스처 필터링(Point/Linear), 어드레싱 모드(Wrap/Clamp)

**선행 지식 — CS/수학**
- UV 좌표계(0~1), 텍셀 vs 픽셀, 색 공간(sRGB vs linear) 기초

**추천 자료**: Luna 9장, `D3D12HelloTexture`

**완료 기준**: 디스크에서 PNG를 읽어 사각형에 입혀 출력되고, 필터/어드레싱 모드 변경이 반영된다.

---

## Phase 6 — 2D 스프라이트 렌더링 & 배칭 (★ 2D 엔진의 심장)

**목표**: 직교 투영 기반으로 스프라이트(쿼드)를 수천 개 효율적으로 그린다.

**구현할 기능**
- **직교(Orthographic) 투영 행렬** + 2D 카메라(위치/줌)
- 스프라이트 = 위치/크기/회전/UV/색을 가진 데이터
- **스프라이트 배칭(Sprite Batch)**: 같은 텍스처/상태의 스프라이트를 동적 정점 버퍼에 모아 1회 Draw
- 상수 버퍼로 카메라 행렬 전달, 인스턴싱 또는 동적 버퍼 갱신
- 알파 블렌딩(투명 PNG) 설정

**선행 지식 — DirectX**
- 동적(Dynamic) 정점 버퍼 갱신 패턴, 프레임당 버퍼 재사용(ring)
- 블렌드 스테이트(소스/대상 팩터), 그리기 순서와 투명도
- 인스턴싱 vs 배치 버퍼 트레이드오프

**선행 지식 — 수학(중요)**
- **2D 변환 행렬**: 이동/회전/스케일과 그 합성(TRS), 행렬 곱 순서
- **직교 투영 행렬 유도** (월드 → NDC), 좌표계(픽셀 ↔ 월드 ↔ 클립)
- 동차 좌표(homogeneous coordinates)

**선행 지식 — CS**
- 드로우콜 비용과 배칭이 성능에 미치는 영향(CPU 바운드 이해)

**추천 자료**: Luna 행렬/카메라 장, learnopengl.com "2D Game" 시리즈(개념 이식), MonoGame SpriteBatch 설계 참고

**완료 기준**: 회전/스케일된 투명 스프라이트 수백~수천 개가 한 자릿수 드로우콜로 60fps에 그려진다.

---

## Phase 7 — 자작 수학 라이브러리

**목표**: 엔진 전반에 쓸 벡터/행렬 수학을 직접 구현하며 내부 동작을 이해한다.
(Phase 6와 병행/직전 진행 가능)

**구현할 기능**
- `Vector2/3/4`, `Matrix4x4`, `Quaternion`(2D는 회전각으로 충분하나 확장 대비)
- 내적/외적/정규화/길이, 행렬 곱·역행렬·전치
- 투영/뷰 행렬 헬퍼, 변환 합성
- 단위 테스트 (값 검증)

**선행 지식 — 수학**
- 선형대수: 벡터 공간, 내적/외적의 기하학적 의미, 행렬을 "좌표 변환"으로 이해
- 행벡터 vs 열벡터 / row-major vs column-major (D3D/HLSL 관례와 일치시키기)

**선행 지식 — CS**
- SIMD 개념(직접 구현은 스칼라로 하되, `DirectXMath`가 왜 빠른지 이해), 부동소수점 오차

**추천 자료**: 📘 *3D Math Primer for Graphics and Game Development* (Dunn & Parberry), Luna 1~2장

**완료 기준**: 자작 수학으로 Phase 6 카메라/변환이 동일하게 동작하고, 테스트가 통과한다.

---

## Phase 8 — 입력 시스템

**목표**: 키보드/마우스 입력을 추상화하고 게임 로직에서 폴링한다.

**구현할 기능**
- Win32 메시지(`WM_KEYDOWN/UP`, `WM_MOUSEMOVE`) → 입력 상태 테이블
- "현재 눌림 / 이번 프레임에 눌림(pressed) / 떼어짐(released)" 상태 구분
- 마우스 좌표를 월드 좌표로 변환(Phase 6 카메라 역변환)
- (선택) 게임패드 — XInput

**선행 지식 — 시스템/CS**
- 메시지 펌프와 게임 루프 프레임 경계의 관계, 입력 버퍼링/에지 검출
- 입력 ↔ 시뮬레이션 분리(추상 액션 매핑) 설계

**추천 자료**: learn.microsoft.com Raw Input / XInput, 일반 게임 입력 패턴 글

**완료 기준**: 키/마우스로 스프라이트를 이동시키고, "한 번 누름"과 "누르고 있음"이 구분된다.

---

## Phase 9 — 게임 루프 & 타이밍

**목표**: 프레임 독립적이고 결정적인 업데이트 루프를 만든다.

**구현할 기능**
- `QueryPerformanceCounter` 기반 고정밀 타이머, delta time 계산
- **고정 타임스텝(fixed timestep)** + 렌더 보간, 또는 가변 타임스텝(트레이드오프 이해)
- FPS 측정/표시, 일시정지/프레임 스킵 처리

**선행 지식 — CS**
- 시뮬레이션 결정성, 가변 vs 고정 타임스텝의 물리/네트워크 영향
- 스파이럴 오브 데스(spiral of death) 방지

**추천 자료**: 🌐 Glenn Fiedler "Fix Your Timestep!", Gregory 책 타이밍 장

**완료 기준**: 무거운 프레임에도 이동 속도가 일정하고, 타임스텝 변경 시 동작이 안정적이다.

---

## Phase 10 — 스프라이트 시트 & 애니메이션

**목표**: 한 텍스처(아틀라스) 안의 프레임을 잘라 애니메이션을 재생한다.

**구현할 기능**
- 텍스처 아틀라스 / 스프라이트 시트 UV 분할
- 프레임 애니메이션(프레임 배열 + 속도 + 루프)
- (선택) 아틀라스 메타데이터(JSON) 로딩

**선행 지식**: UV 부분 영역 매핑, 시간 기반 상태 갱신(Phase 9 delta 활용), 텍스처 빈 패킹 개념

**추천 자료**: TexturePacker 포맷, 일반 2D 애니메이션 튜토리얼

**완료 기준**: 걷기 등 다중 프레임 애니메이션이 일정 속도로 루프 재생된다.

---

## Phase 11 — 텍스트 렌더링 (디버그/UI)

**목표**: 화면에 텍스트를 그린다(디버그 출력 → UI 기반).

**구현할 기능**
- 비트맵 폰트 아틀라스 방식(간단) 또는 `stb_truetype`로 글리프 아틀라스 생성
- 글자별 쿼드를 스프라이트 배치로 렌더(Phase 6 재사용)

**선행 지식**: 폰트 메트릭(베이스라인, advance, kerning 기초), 글리프 아틀라스 패킹, 텍스트 레이아웃

**추천 자료**: stb_truetype 예제, "font atlas rendering" 자료

**완료 기준**: FPS/디버그 정보가 화면에 텍스트로 출력된다.

---

## Phase 12 — 씬 / 엔티티 구조

**목표**: 게임 오브젝트를 조직화하는 아키텍처를 도입한다.

**구현할 기능**
- 단순 엔티티-컴포넌트 또는 경량 **ECS**(Transform, Sprite, Velocity 등 컴포넌트)
- 씬: 엔티티 컬렉션 + 업데이트/렌더 순회
- 생성/파괴, 부모-자식 변환(선택)

**선행 지식 — CS(중요)**
- 컴포지션 vs 상속, 데이터 지향 설계(DOD)와 캐시 지역성, ECS의 동기
- 메모리 풀/오브젝트 풀, 핸들 기반 참조

**추천 자료**: 🌐 *Game Programming Patterns* (Robert Nystrom, 무료 온라인) — Component, Update, Object Pool 패턴, Gregory 책

**완료 기준**: 여러 엔티티가 컴포넌트로 정의되고 씬이 일괄 업데이트/렌더한다.

---

## Phase 13 — 2D 충돌 & 기초 물리

**목표**: 게임 상호작용의 토대인 충돌 판정을 만든다.

**구현할 기능**
- AABB / 원 충돌 판정, 충돌 응답(밀어내기)
- 속도/가속도 기반 이동, 중력(필요 시)
- (선택) 공간 분할(그리드/쿼드트리)로 브로드페이즈 최적화

**선행 지식 — 수학/CS**
- AABB·원·SAT 충돌 알고리즘, 벡터 반사/투영
- 브로드페이즈 vs 내로우페이즈, 적분(오일러) 기초

**추천 자료**: 🌐 *Game Physics Engine Development* 일부, "2D collision detection" 가이드, Real-Time Collision Detection(심화)

**완료 기준**: 캐릭터가 벽/플랫폼과 충돌하고 적절히 멈추거나 반응한다.

---

## Phase 14 — 오디오

**목표**: 효과음/배경음 재생.

**구현할 기능**
- **XAudio2** 초기화, WAV 로딩, 사운드 보이스 재생/볼륨/루프
- 사운드 매니저(동시 재생, 풀링)

**선행 지식**: PCM 오디오 기초(샘플레이트/채널), XAudio2 소스 보이스 모델

**추천 자료**: learn.microsoft.com XAudio2, DirectXTK Audio(참고용)

**완료 기준**: 입력/충돌에 맞춰 효과음이 재생되고 BGM이 루프된다.

---

## Phase 15 — 에셋 / 리소스 매니저

**목표**: 텍스처/사운드/폰트를 중복 없이 로딩·캐싱·해제한다.

**구현할 기능**
- 경로 기반 캐시(이미 로드된 리소스 재사용), 참조 카운팅/수명 관리
- (선택) 간단한 에셋 메타(JSON) / 핫리로드

**선행 지식 — CS**: 리소스 핸들, 캐싱 전략, 비동기 로딩(선택), 직렬화 기초

**완료 기준**: 같은 텍스처를 여러 곳에서 써도 한 번만 로드되고 종료 시 누수가 없다.

---

## Phase 16 — 디버그 / 에디터 UI

**목표**: 런타임 값 조정·디버깅 도구를 붙여 개발 속도를 끌어올린다.

**구현할 기능**
- **Dear ImGui** 통합(D3D12 백엔드) — 엔티티 인스펙터, 성능 그래프, 토글
- 디버그 드로잉(충돌 박스, 좌표축)

**선행 지식 — DirectX**: 외부 렌더 백엔드를 내 커맨드 리스트/디스크립터 힙과 공존시키는 법

**추천 자료**: Dear ImGui 공식 D3D12 예제

**완료 기준**: 실행 중 값(속도, 색 등)을 UI로 조정하면 즉시 반영된다.

---

## Phase 17 — 캡스톤: 첫 2D 게임 완성 (★ 최종 목표)

**목표**: 지금까지의 시스템을 통합해 **완결성 있는 작은 2D 게임 한 편**을 만든다.
(예: 플랫포머, 탑다운 슈터, 벽돌깨기급 — 스코프를 작게)

**통합 요소**
- 씬 전환(타이틀 → 플레이 → 게임오버), 게임 상태 머신
- 스프라이트/애니메이션/충돌/입력/오디오/UI 전부 결합
- 점수/HP 등 게임플레이 루프, 간단한 레벨 데이터 로딩
- 빌드 패키징(릴리스 빌드 + 에셋 폴더 배포)

**선행 지식 — 게임 디자인**
- 코어 게임 루프, 상태 머신, 난이도/페이싱, 최소 기획 범위 관리

**완료 기준**: 빌드된 실행 파일로 처음부터 끝까지 플레이 가능하고, 누수/크래시 없이 종료된다.

---

## (선택) Phase 18 — 심화 주제 (이후 확장)

엔진을 더 발전시키고 싶을 때의 다음 단계들:
- **고급 D3D12**: Bundle, GPU-driven 렌더링, 멀티스레드 커맨드 기록, 디스크립터 힙 전략 고도화
- **렌더링**: 2D 라이팅/노멀맵, 파티클 시스템, 포스트 프로세싱, 셰이더 효과
- **타일맵 & 레벨 에디터**, 카메라 효과(셰이크/패럴랙스)
- **세이브/로드 직렬화**, 스크립팅(Lua) 바인딩
- **3D 확장** 또는 D3D12 기반 신기능(메시 셰이더 등)

---

## 학습 운영 원칙 (전체 관통)

1. **각 Phase = "선행 지식 학습 → 작은 구현 → PIX/디버거로 검증 → Git 커밋"** 사이클로 진행.
2. **PIX 캡처를 습관화** — D3D12는 화면이 검게 나와도 원인을 눈으로 봐야 한다.
3. **디버그 레이어 경고는 0을 유지** — 경고를 미루면 나중에 원인 추적이 불가능해진다.
4. 막히면 **Microsoft 공식 샘플의 동일 단계**와 내 코드를 비교.
5. 개념이 흔들리면 D3D11 자료(rastertek 등)로 **개념만** 먼저 잡고 D3D12로 옮겨와도 좋다.
6. **스코프 관리** — 엔진은 "지금 만들 게임에 필요한 만큼만". 완벽주의가 가장 큰 적.

---

## 권장 프로젝트 디렉토리 구조 (Phase 2부터 점진 정착)

```
DKEngine/
├─ ROADMAP.md
├─ src/
│  ├─ Core/        (Application, Window, Timer, Input)
│  ├─ RHI/         (Device, SwapChain, CommandQueue, Buffer, Texture, PSO ...) — D3D12 추상화
│  ├─ Renderer/    (SpriteBatch, Camera2D, Shader)
│  ├─ Math/        (Vector, Matrix)
│  ├─ Scene/       (Entity/ECS, Components)
│  ├─ Audio/
│  └─ main.cpp
├─ shaders/        (*.hlsl)
├─ assets/         (textures, sounds, fonts)
├─ third_party/    (stb, imgui, ...)
└─ docs/
```

---

## 진행 체크리스트

- [x] Phase 0 — 핵심 빌드 도구 확인 (VS2022, MSVC, Win SDK) & 빈 프로젝트 생성
- [x] PIX 설치 확인
- [ ] PIX GPU Capture로 Phase 2 clear/present 흐름 확인
- [x] Phase 1 — Win32 윈도우 & 메시지 루프
- [x] Phase 2 — D3D12 초기화 (화면 클리어)
- [ ] Phase 3 — 첫 삼각형
- [ ] Phase 4 — 렌더링 추상화 계층
- [ ] Phase 5 — 텍스처 & 리소스 업로드
- [ ] Phase 6 — 2D 스프라이트 렌더링 & 배칭
- [ ] Phase 7 — 자작 수학 라이브러리
- [ ] Phase 8 — 입력 시스템
- [ ] Phase 9 — 게임 루프 & 타이밍
- [ ] Phase 10 — 스프라이트 시트 & 애니메이션
- [ ] Phase 11 — 텍스트 렌더링
- [ ] Phase 12 — 씬 / 엔티티 구조
- [ ] Phase 13 — 2D 충돌 & 기초 물리
- [ ] Phase 14 — 오디오
- [ ] Phase 15 — 에셋 / 리소스 매니저
- [ ] Phase 16 — 디버그 / 에디터 UI
- [ ] Phase 17 — 캡스톤: 첫 2D 게임 완성
