# Debug Layer, DXGI Factory, Adapter

## 1. Debug Layer

D3D12 Debug Layer는 DirectX 12 사용 오류를 실행 중에 알려주는 진단 레이어다.

예를 들어 다음 같은 문제를 잡아준다.

- 리소스 상태 전이를 빼먹음
- GPU가 아직 쓰는 리소스를 해제함
- 잘못된 descriptor를 사용함
- command list 사용 순서가 틀림
- swap chain, resource, descriptor heap 사용 방식이 잘못됨

D3D12는 D3D11보다 개발자가 직접 관리해야 하는 것이 많다. 그래서 Debug Layer를 켜지 않으면
화면이 검게 나오거나 아무것도 안 그려져도 원인을 찾기 어렵다.

## 2. 현재 코드에서 쓰인 위치

```text
src/RHI/D3D12Renderer.cpp
└─ D3D12Renderer::EnableDebugLayer()
```

현재 코드:

```cpp
D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
debugController->EnableDebugLayer();
```

의도:

- Debug 빌드에서만 D3D12 Debug Layer를 켠다.
- Release 빌드는 사용자가 실행하는 최종 빌드에 가까우므로 디버그 레이어를 켜지 않는다.
- Debug Layer가 없어도 프로그램 실행은 가능하게 했다. 그래서 실패해도 경고만 남기고 계속 진행한다.

## 3. DXGI란?

DXGI는 **DirectX Graphics Infrastructure**의 약자다.

D3D12 자체가 GPU 명령과 리소스를 다루는 API라면, DXGI는 그보다 바깥쪽에서 다음을 담당한다.

- GPU 어댑터 열거
- 모니터/출력 관련 정보
- 스왑체인 생성
- 백버퍼를 화면에 Present하는 기반

즉 D3D12 Device를 만들고 그림을 그리더라도, 그 결과를 Win32 창에 보여주려면 DXGI SwapChain이 필요하다.

## 4. DXGI Factory

DXGI Factory는 DXGI 객체를 만드는 출발점이다.

현재 코드에서 하는 일:

```text
D3D12Renderer::CreateFactory()
```

```cpp
CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));
```

이 Factory는 이후 두 가지 핵심 작업에 쓰인다.

1. GPU 어댑터 열거

```cpp
m_factory->EnumAdapters1(...)
```

2. Win32 창용 SwapChain 생성

```cpp
m_factory->CreateSwapChainForHwnd(...)
```

## 5. Adapter

Adapter는 DXGI가 바라보는 GPU 장치다.

PC에는 여러 adapter가 있을 수 있다.

- 내장 GPU
- 외장 GPU
- Microsoft Basic Render Driver 같은 소프트웨어 어댑터
- 원격/가상 디스플레이 어댑터

현재 코드는 다음 정책으로 adapter를 고른다.

```text
소프트웨어 어댑터 제외
-> D3D12CreateDevice가 성공하는 첫 하드웨어 어댑터 선택
```

코드 위치:

```text
D3D12Renderer::CreateDevice()
```

의도:

- Phase 2에서는 고성능 GPU 선택 정책보다 "D3D12가 실제로 동작하는 경로"가 중요하다.
- 나중에 필요하면 전용 GPU 우선, VRAM 큰 GPU 우선 같은 정책을 추가할 수 있다.

## 6. 실수하기 쉬운 점

- Debug Layer를 Device 생성 후에 켜려고 하면 늦다. Device 생성 전에 켜야 한다.
- Debug DXGI Factory 생성이 실패할 수 있다. 현재 코드는 Debug Factory 생성 실패 시 일반 Factory로 재시도한다.
- Software Adapter를 선택하면 성능이나 기능이 기대와 다를 수 있다.
- Adapter 선택과 Device 생성은 연결되어 있다. 선택한 adapter로 `D3D12CreateDevice`를 호출해야 한다.

## 7. 앞으로 확장될 부분

Phase 3 이후에도 Debug Layer는 계속 켜둔다.

나중에 추가할 수 있는 것:

- DXGI 1.6의 `EnumAdapterByGpuPreference` 사용
- 전용 GPU 우선 선택
- adapter 이름, VRAM 크기 로그 출력
- tearing 지원 여부 확인
- PIX 캡처 시 객체 이름 더 자세히 설정
