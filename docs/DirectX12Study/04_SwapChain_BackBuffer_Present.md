# SwapChain, Back Buffer, Present

## 1. SwapChain이란?

SwapChain은 화면에 표시할 이미지 버퍼들을 관리하는 DXGI 객체다.

게임은 보통 화면에 직접 그리지 않는다.
뒤쪽 버퍼(back buffer)에 먼저 그리고, 다 그린 뒤 `Present`로 화면에 표시한다.

Phase 2에서는 다음 구조를 사용한다.

```text
SwapChain
├─ Back Buffer 0
└─ Back Buffer 1
```

현재 `kFrameCount = 2`이므로 더블 버퍼링이다.

## 2. Back Buffer

Back Buffer는 다음 프레임을 그릴 대상 이미지다.

현재 코드에서는 SwapChain에서 back buffer를 가져온다.

```cpp
m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
```

이 리소스에 RTV를 만들면 `ClearRenderTargetView`로 지울 수 있다.

## 3. Present

`Present`는 현재 back buffer를 화면에 표시하는 호출이다.

현재 코드:

```cpp
m_swapChain->Present(1, 0);
```

첫 번째 인자 `1`은 V-Sync에 맞춰 표시하겠다는 뜻이다.

의도:

- 화면 찢김(tearing)을 피한다.
- Phase 2에서는 성능 측정보다 안정적인 화면 표시를 우선한다.

## 4. 현재 코드에서 쓰인 위치

SwapChain 생성:

```text
D3D12Renderer::CreateSwapChain()
```

Present 호출:

```text
D3D12Renderer::Render()
```

현재 SwapChain 설정:

```cpp
swapChainDesc.BufferCount = kFrameCount;
swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
```

## 5. FLIP_DISCARD

`DXGI_SWAP_EFFECT_FLIP_DISCARD`는 현대 DXGI에서 권장되는 flip model swap effect다.

의미:

- Present 시 back buffer가 화면에 flip된다.
- 이전 back buffer 내용은 보존하지 않는다.
- 매 프레임 전체 화면을 다시 그리는 게임/엔진에 적합하다.

Phase 2에서는 매 프레임 전체를 clear하므로 이전 내용 보존이 필요 없다.

## 6. 현재 Back Buffer 인덱스

SwapChain은 현재 어느 back buffer에 그려야 하는지 알려준다.

```cpp
m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
```

이 값은 RTV handle 계산에 쓰인다.

```cpp
rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
```

의도:

- 현재 그릴 back buffer와 그 back buffer의 RTV를 맞춘다.
- 잘못된 인덱스를 쓰면 다른 buffer를 clear하거나 잘못된 RTV를 사용할 수 있다.

## 7. Win32 HWND와 SwapChain

현재 코드는 Win32 창에 붙는 swap chain을 만든다.

```cpp
m_factory->CreateSwapChainForHwnd(
    m_commandQueue.Get(),
    hwnd,
    &swapChainDesc,
    nullptr,
    nullptr,
    &swapChain1);
```

SwapChain은 command queue와 HWND를 모두 필요로 한다.

- command queue: 어떤 GPU queue에서 present 흐름을 관리할지
- HWND: 결과를 어느 창에 보여줄지

## 8. Alt+Enter 비활성화

현재 코드:

```cpp
m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
```

의도:

- DXGI 기본 Alt+Enter 전체화면 전환을 막는다.
- 나중에 엔진이 직접 전체화면/창모드 정책을 관리할 수 있게 한다.

## 9. 실수하기 쉬운 점

- back buffer는 render target으로 쓰기 전에 RTV가 필요하다.
- Present 전에는 resource state를 `PRESENT`로 돌려야 한다.
- `GetCurrentBackBufferIndex()`와 RTV handle 계산이 맞아야 한다.
- 창 크기가 바뀌면 SwapChain back buffer도 리사이즈해야 한다.
- `FLIP_DISCARD`에서는 이전 프레임 내용 보존을 기대하면 안 된다.

## 10. 앞으로 확장될 부분

Phase 3:

- Back Buffer에 clear뿐 아니라 triangle draw 결과가 들어간다.

Phase 4:

- SwapChain 래퍼 클래스로 분리
- tearing 지원 확인
- frame latency 관리

Phase 6 이후:

- 스프라이트 렌더 결과를 swap chain back buffer에 출력
