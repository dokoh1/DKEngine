# Fence, CPU/GPU Synchronization

## 1. 왜 동기화가 필요한가?

D3D12에서 CPU와 GPU는 같은 속도로 움직이지 않는다.

CPU는 command list를 빠르게 기록하고 queue에 제출한다.
GPU는 그 명령을 나중에 실행한다.

즉 CPU가 어떤 코드를 지나갔다고 해서 GPU 작업이 끝난 것은 아니다.

문제가 되는 상황:

- GPU가 아직 back buffer를 쓰는 중인데 CPU가 back buffer를 해제함
- GPU가 command allocator의 명령 메모리를 읽는 중인데 CPU가 allocator를 Reset함
- ResizeBuffers를 호출해야 하는데 기존 back buffer를 GPU가 사용 중임

이런 문제를 막기 위해 Fence를 사용한다.

## 2. Fence란?

Fence는 GPU 진행 지점을 숫자로 표시하는 동기화 객체다.

CPU가 queue에 이렇게 요청한다.

```text
GPU야, 여기까지 실행하면 fence 값을 N으로 만들어라.
```

그 후 CPU는 fence 값이 N 이상이 되었는지 확인한다.

## 3. 현재 코드에서 쓰인 위치

생성:

```text
D3D12Renderer::CreateFence()
```

대기:

```text
D3D12Renderer::WaitForGpu()
```

종료:

```text
D3D12Renderer::Shutdown()
```

리사이즈:

```text
D3D12Renderer::Resize()
```

## 4. Fence 생성

현재 코드:

```cpp
m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
```

함께 event도 만든다.

```cpp
m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
```

Event가 필요한 이유:

- CPU가 계속 반복문으로 기다리면 CPU를 낭비한다.
- `SetEventOnCompletion`을 사용하면 fence 완료 시 event가 신호 상태가 된다.
- CPU는 `WaitForSingleObject`로 효율적으로 기다릴 수 있다.

## 5. WaitForGpu 흐름

현재 코드의 핵심:

```cpp
const unsigned long long fenceToWaitFor = ++m_fenceValue;
m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor);

if (m_fence->GetCompletedValue() < fenceToWaitFor)
{
    m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
}
```

의미:

1. fence 값을 하나 증가시킨다.
2. command queue에 signal 명령을 넣는다.
3. GPU가 그 signal 지점까지 실행하면 fence completed value가 증가한다.
4. CPU는 completed value를 확인한다.
5. 아직 도달하지 않았으면 event로 기다린다.

## 6. Phase 2에서 매 프레임 기다리는 이유

현재 `Render()`는 `Present()` 후 `WaitForGpu()`를 호출한다.

장점:

- 이해하기 쉽다.
- command allocator 하나를 안전하게 재사용할 수 있다.
- Resize/Shutdown이 단순해진다.
- Phase 2 디버깅이 쉽다.

단점:

- CPU/GPU 병렬성이 줄어든다.
- 성능상 최적은 아니다.

Phase 2의 목표는 성능이 아니라 D3D12 최소 실행 경로를 정확히 이해하는 것이다.

## 7. Shutdown에서 Fence가 필요한 이유

종료 시에는 ComPtr를 Reset해서 D3D12 리소스를 해제한다.

하지만 GPU가 아직 그 리소스를 사용 중이면 문제가 생긴다.

그래서 `Shutdown()` 시작에서:

```cpp
WaitForGpu();
```

를 호출한다.

의도:

- GPU가 모든 제출된 작업을 끝냈는지 보장
- 그 후 back buffer, command list, queue, device를 안전하게 해제

## 8. Resize에서 Fence가 필요한 이유

`ResizeBuffers`를 호출하려면 기존 back buffer 참조를 해제해야 한다.

하지만 GPU가 아직 back buffer를 사용 중일 수 있다.

그래서 Resize 순서는:

```text
WaitForGpu
ReleaseRenderTargets
ResizeBuffers
CreateRenderTargetViews
```

## 9. 실수하기 쉬운 점

- `ExecuteCommandLists` 후 바로 GPU 작업이 끝났다고 생각하면 안 된다.
- Fence 없이 command allocator를 Reset하면 위험하다.
- Fence 없이 back buffer를 Release/Resize하면 위험하다.
- fence value는 계속 증가해야 한다.
- event handle은 종료 시 `CloseHandle`로 닫아야 한다.

## 10. 앞으로 확장될 부분

Phase 4:

- 프레임별 fence value
- 프레임 리소스 구조
- CPU/GPU 병렬 실행
- allocator를 frame count만큼 분리

Phase 6:

- dynamic vertex buffer ring과 fence 기반 재사용

Phase 15 이후:

- 비동기 리소스 업로드와 copy queue 동기화
