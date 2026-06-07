# ResizeBuffers and Window Resize

## 1. 왜 리사이즈 처리가 필요한가?

Win32 창의 클라이언트 영역 크기가 바뀌면 SwapChain의 back buffer 크기도 바뀌어야 한다.

창은 1600x900인데 back buffer가 계속 1280x720이면:

- 화면이 늘어나 보일 수 있다.
- 렌더 타겟 크기와 창 크기가 맞지 않는다.
- viewport/scissor를 추가할 때 계산이 꼬일 수 있다.

Phase 2에서는 창 크기가 바뀌면 back buffer를 같은 크기로 다시 만든다.

## 2. 현재 코드 흐름

Win32 메시지:

```text
Window::HandleMessage()
└─ WM_SIZE
   └─ m_width, m_height 갱신
```

Application:

```text
Application::Render()
└─ m_renderer.Render(m_window.Width(), m_window.Height())
```

Renderer:

```text
D3D12Renderer::Render(width, height)
└─ if width/height changed
   └─ Resize(width, height)
```

## 3. Resize 순서

현재 코드:

```text
D3D12Renderer::Resize()
```

순서:

1. `WaitForGpu()`
2. `ReleaseRenderTargets()`
3. `m_swapChain->ResizeBuffers(...)`
4. 새 width/height 저장
5. 현재 back buffer index 갱신
6. `CreateRenderTargetViews()`

이 순서가 중요하다.

## 4. WaitForGpu를 먼저 하는 이유

GPU가 기존 back buffer를 사용 중일 수 있다.

예:

```text
CPU: ResizeBuffers 호출하고 싶음
GPU: 이전 프레임 back buffer clear/present 중
```

이때 바로 back buffer를 해제하면 위험하다.

그래서 먼저:

```cpp
WaitForGpu();
```

로 GPU 작업 완료를 보장한다.

## 5. ReleaseRenderTargets가 필요한 이유

DXGI `ResizeBuffers`는 기존 back buffer에 대한 참조가 남아 있으면 실패할 수 있다.

현재 back buffer 참조:

```cpp
Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[kFrameCount];
```

해제:

```cpp
for (unsigned int i = 0; i < kFrameCount; ++i)
    m_renderTargets[i].Reset();
```

의도:

- 기존 back buffer COM 참조를 해제
- DXGI가 새 크기의 back buffer를 만들 수 있게 함

## 6. ResizeBuffers

현재 코드:

```cpp
m_swapChain->ResizeBuffers(
    kFrameCount,
    static_cast<unsigned int>(width),
    static_cast<unsigned int>(height),
    desc.BufferDesc.Format,
    desc.Flags);
```

의도:

- back buffer 개수는 그대로 유지
- 새 창 크기를 back buffer 크기로 사용
- 기존 swap chain format과 flags 유지

## 7. RTV를 다시 만드는 이유

ResizeBuffers 후 기존 back buffer 리소스는 더 이상 유효하지 않다.

새 back buffer를 다시 가져와야 한다.

```cpp
m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
```

그리고 새 리소스에 대해 RTV를 다시 만든다.

```cpp
m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
```

즉 Resize 후에는 다음이 모두 새로 맞춰져야 한다.

- back buffer resource
- RTV descriptor
- frame index
- 저장된 width/height

## 8. 최소화 처리

창이 최소화되면 width나 height가 0이 될 수 있다.

0 크기로 ResizeBuffers나 렌더 타겟 생성은 할 수 없다.

현재 코드:

```cpp
if (width <= 0 || height <= 0)
    return true;
```

의도:

- 최소화 중에는 렌더링을 건너뛴다.
- 창이 복원되어 유효한 크기가 들어오면 다시 Resize한다.

## 9. 실수하기 쉬운 점

- GPU 완료를 기다리지 않고 ResizeBuffers 호출
- 기존 back buffer ComPtr를 Reset하지 않음
- ResizeBuffers 후 RTV를 다시 만들지 않음
- frame index를 갱신하지 않음
- width/height가 0일 때 ResizeBuffers 호출
- WM_SIZE 안에서 직접 D3D12 리사이즈를 수행해 메시지 처리와 렌더러 책임이 섞임

## 10. 앞으로 확장될 부분

Phase 3:

- back buffer 크기에 맞춰 viewport/scissor도 갱신해야 한다.

Phase 4:

- SwapChain 클래스로 리사이즈 로직 분리
- Renderer가 resize event를 명시적으로 받는 구조 검토

Phase 6:

- 2D 카메라 projection을 창 크기에 맞게 갱신

Phase 16:

- ImGui viewport 또는 editor panel 크기에 맞춘 렌더 타겟 관리
