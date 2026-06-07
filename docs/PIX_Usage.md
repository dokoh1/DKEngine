# PIX on Windows 사용법

## 1. 현재 설치 상태

현재 PC에서 PIX 설치가 확인됐다.

```text
C:\Program Files\Microsoft PIX
C:\Program Files\Microsoft PIX\2603.25\WinPix.exe
```

설치 확인 명령:

```powershell
Test-Path "C:\Program Files\Microsoft PIX"
```

`True`가 나오면 설치 경로가 잡힌 것이다.

---

## 2. PIX를 쓰는 목적

PIX는 DirectX 12 프로그램의 **GPU 디버깅/프로파일링 도구**다.

Visual Studio 디버거는 CPU 코드가 어디까지 실행됐는지 보는 데 강하고, PIX는 GPU가 실제로 어떤
D3D12 명령을 받았고 어떤 리소스를 사용했는지 보는 데 강하다.

DKEngine에서 PIX를 쓰는 이유:

- `ClearRenderTargetView`가 실제 GPU command list에 기록됐는지 확인
- 백버퍼 상태 전이(`Present -> RenderTarget -> Present`)가 맞는지 확인
- 현재 RTV가 어떤 백버퍼를 가리키는지 확인
- Phase 3 이후 draw call, vertex buffer, shader, PSO 문제를 추적
- 나중에 스프라이트 렌더링 성능을 GPU 타이밍으로 확인

---

## 3. Phase 2에서 확인할 것

Phase 2의 목표는 화면을 단색으로 지우고 `Present`하는 것이다.

PIX GPU Capture에서 한 프레임 안에 다음 흐름이 보이면 정상이다.

```text
ResourceBarrier: Present -> RenderTarget
ClearRenderTargetView
ResourceBarrier: RenderTarget -> Present
Present
```

우리 코드 기준 위치:

```text
src/RHI/D3D12Renderer.cpp
└─ D3D12Renderer::Render()
```

---

## 4. GPU Capture 찍는 방법

### 방법 A. PIX에서 DKEngine 실행

1. 시작 메뉴에서 **PIX on Windows** 실행
2. 첫 화면의 **PC Connection** 화면에서 현재 PC 선택
3. **Launch Win32** 또는 실행 대상 설정 영역에서 DKEngine 실행 파일 지정

```text
C:\Users\dko00\DKEngine\build\x64\Debug\DKEngine.exe
```

4. Working directory는 프로젝트 루트로 지정

```text
C:\Users\dko00\DKEngine
```

5. **Launch for GPU Capture** 옵션으로 실행
6. DKEngine 창이 뜨면 PIX에서 **GPU Capture** 버튼을 누르거나, 게임 창에 포커스를 두고 `PrintScreen` 키 입력
7. 캡처가 열리면 상단의 **Start Analysis**를 눌러 분석 시작

### 방법 B. 이미 실행 중인 프로세스에 Attach

가능은 하지만 지금 프로젝트에서는 권장하지 않는다. GPU Capture attach는 D3D12 API 호출 전에 PIX 캡처 DLL이
로드되어 있어야 하는 조건이 있다. 초기 엔진 단계에서는 PIX가 직접 실행하게 하는 방법 A가 더 단순하다.

---

## 5. 캡처에서 볼 화면

### Events

GPU에 제출된 D3D12 API 호출 목록을 본다.

Phase 2에서 확인할 이벤트:

- `ID3D12GraphicsCommandList::ResourceBarrier`
- `ID3D12GraphicsCommandList::ClearRenderTargetView`
- `ID3D12CommandQueue::ExecuteCommandLists`
- `IDXGISwapChain::Present`

필요하면 Events view의 필터에 `Clear` 또는 `Barrier`를 입력해 찾는다.

### Event Details

선택한 이벤트의 파라미터를 본다.

확인할 것:

- `ClearRenderTargetView`의 clear color가 코드와 같은지
- Resource Barrier의 `StateBefore`, `StateAfter`가 맞는지
- RTV handle이 현재 frame index에 맞는지

### Pipeline / Render Target

현재 백버퍼가 실제로 어떤 색으로 clear됐는지 본다.

현재 clear color:

```cpp
const float clearColor[] = { 0.08f, 0.16f, 0.28f, 1.0f };
```

화면이 어두운 파란색 계열로 채워져 있으면 Phase 2 렌더 경로가 정상이다.

---

## 6. Timing Capture는 언제 쓰나

지금 Phase 2에서는 GPU Capture가 우선이다.

Timing Capture는 나중에 성능을 볼 때 사용한다.

예:

- 프레임 시간이 어디서 길어지는지
- CPU와 GPU 작업이 어느 시점에 실행되는지
- command queue 작업 시간이 얼마나 걸리는지
- 스프라이트 수가 많을 때 GPU 병목인지 CPU 병목인지

Phase 6 이후 스프라이트 배칭이나 대량 렌더링을 만들 때 본격적으로 쓴다.

---

## 7. 문제가 생겼을 때 보는 순서

### 화면이 검게 나올 때

1. `D3D12Renderer::Render()`가 호출되는지 로그 확인
2. PIX GPU Capture에서 `ClearRenderTargetView`가 있는지 확인
3. `ResourceBarrier`가 `Present -> RenderTarget`, `RenderTarget -> Present` 순서인지 확인
4. RTV가 현재 백버퍼에 대해 생성됐는지 확인
5. Visual Studio 출력 창에서 D3D12 Debug Layer 경고 확인

### 캡처가 안 될 때

1. Debug 빌드로 실행했는지 확인
2. PIX에서 직접 `DKEngine.exe`를 Launch했는지 확인
3. 실행 파일 경로가 `build\x64\Debug\DKEngine.exe`인지 확인
4. D3D12 Debug Layer 경고가 있는지 확인
5. 그래픽 드라이버가 너무 오래되지 않았는지 확인

---

## 8. 앞으로의 사용 기준

- Phase 2: 백버퍼 clear/present 흐름 확인
- Phase 3: draw call, vertex buffer, shader, PSO 확인
- Phase 5: texture upload, SRV, sampler 확인
- Phase 6: sprite batch draw call 수와 GPU timing 확인
- Phase 16: ImGui/debug drawing과 엔진 렌더 패스 공존 확인

---

## 참고 자료

- Microsoft PIX 소개: https://devblogs.microsoft.com/pix/introduction/
- PIX로 캡처하기: https://devblogs.microsoft.com/pix/taking-a-capture/
- GPU Captures: https://devblogs.microsoft.com/pix/gpu-captures/
- Timing Captures: https://devblogs.microsoft.com/pix/timing-captures/
