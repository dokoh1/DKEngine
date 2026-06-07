# Phase 2 — Direct3D 12 초기화 & 화면 클리어

## 1. 이번 Phase의 목표

Win32 창 위에 D3D12 렌더링 기반을 붙이고, 매 프레임 백버퍼를 지정한 색으로 지운 뒤
`Present`까지 수행한다. 아직 삼각형, 셰이더, 정점 버퍼는 없다.

이 단계의 핵심은 "그리는 법"보다 **GPU에 명령을 기록하고 제출하고 기다리는 최소 흐름**을 이해하는 것이다.

---

## 2. 무엇을 만들었나

| 파일 | 역할 |
|------|------|
| `src/RHI/D3D12Renderer.{h,cpp}` | D3D12 디바이스, 스왑체인, RTV, 커맨드 객체, 펜스 관리 |
| `src/Core/Application.{h,cpp}` | `D3D12Renderer`를 소유하고 초기화/렌더/종료 호출 |
| `DKEngine.vcxproj` | `d3d12.lib`, `dxgi.lib` 링크 추가 |

현재 구조:

```
Application
├─ Window
└─ D3D12Renderer
   ├─ DXGI Factory
   ├─ D3D12 Device
   ├─ Command Queue / Allocator / List
   ├─ SwapChain
   ├─ RTV Descriptor Heap
   ├─ Back Buffers
   └─ Fence + Event
```

---

## 3. 왜 이렇게 했나

### 의도 1. D3D12 코드를 `Application`에 직접 넣지 않음

`Application`은 실행 흐름을 조율하는 객체다. D3D12의 COM 객체와 세부 초기화 순서를 직접 들고 있으면
`Application.cpp`가 빠르게 비대해진다.

그래서 Phase 2부터 `src/RHI/D3D12Renderer`에 D3D12 코드를 모았다. 아직 `Device`, `SwapChain`,
`CommandQueue`로 파일을 세분화하지 않은 이유는 초기 학습 단계에서 흐름을 한눈에 보는 편이 더 낫기 때문이다.
세분화는 Phase 4에서 진행한다.

### 의도 2. 매 프레임 GPU를 기다리는 단순 동기화

현재 `Render()`는 `Present()` 후 `WaitForGpu()`를 호출한다. 이는 CPU/GPU 병렬성을 살리지 못해 성능상
최적은 아니지만, Phase 2에서는 가장 이해하기 쉽고 안전한 형태다.

Phase 4 이후 프레임 리소스 패턴을 도입하면 프레임별 커맨드 할당자와 펜스 값을 분리해 CPU/GPU가
겹쳐 실행되도록 발전시킨다.

### 의도 3. 리사이즈는 렌더러가 직접 처리

`Window`는 `WM_SIZE`에서 현재 클라이언트 크기만 저장한다. `Application::Render()`는 그 크기를
`D3D12Renderer::Render(width, height)`로 넘긴다.

렌더러는 이전 크기와 다르면:

1. GPU 작업 완료 대기
2. 기존 백버퍼 참조 해제
3. `ResizeBuffers`
4. 새 백버퍼로 RTV 재생성

순서로 처리한다.

---

## 4. 렌더링 한 프레임 흐름

한 프레임은 다음 순서로 진행된다.

1. 커맨드 할당자/리스트 리셋
2. 백버퍼 상태 전이: `Present -> RenderTarget`
3. 현재 백버퍼의 RTV 핸들 계산
4. `ClearRenderTargetView`로 단색 클리어
5. 백버퍼 상태 전이: `RenderTarget -> Present`
6. 커맨드 리스트 닫기
7. 커맨드 큐에 제출
8. 스왑체인 `Present`
9. Fence로 GPU 완료 대기
10. 다음 백버퍼 인덱스 갱신

---

## 5. 여기서 배우는 지식

- `ComPtr`로 COM 객체 수명을 관리하는 이유
- DXGI Factory가 어댑터와 스왑체인을 만드는 출발점이라는 점
- D3D12 Device가 리소스/디스크립터/커맨드 객체를 만드는 중심이라는 점
- RTV Descriptor Heap은 백버퍼를 렌더 타겟으로 바라보는 핸들 배열이라는 점
- D3D12 리소스는 사용 목적에 맞게 상태 전이(Resource Barrier)를 명시해야 한다는 점
- GPU 명령은 CPU가 기록하고 큐에 제출한 뒤, Fence로 완료 여부를 확인한다는 점

---

## 6. 완료 기준 / 다음 단계

- [x] `Debug|x64` 빌드 성공
- [x] `Release|x64` 빌드 성공
- [x] 실행 후 정상 종료 코드 0 확인
- [x] D3D12 렌더러가 창 크기 기준으로 스왑체인/RTV를 생성
- [x] 매 프레임 백버퍼 clear + present 수행
- [x] PIX 설치 확인 (`C:\Program Files\Microsoft PIX`)
- [ ] PIX 캡처로 백버퍼 clear 이벤트 확인

다음: **Phase 3 — 첫 삼각형**

Phase 3에서는 셰이더, 루트 시그니처, PSO, 정점 버퍼를 추가해 실제 도형을 그린다.
