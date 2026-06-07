# Command Queue, Command Allocator, Command List

## 1. D3D12의 명령 실행 방식

D3D12는 CPU가 GPU 명령을 즉시 실행시키는 구조가 아니다.

흐름은 다음과 같다.

```text
CPU가 Command List에 명령 기록
-> Command Queue에 제출
-> GPU가 나중에 실행
```

즉 CPU와 GPU는 비동기적으로 동작한다.

Phase 2에서 기록하는 명령:

- Resource Barrier
- ClearRenderTargetView

아직 Draw Call은 없다.

## 2. Command Queue

Command Queue는 GPU에게 실행할 command list를 제출하는 큐다.

현재 코드 위치:

```text
D3D12Renderer::CreateCommandObjects()
```

생성 코드:

```cpp
D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
```

의도:

- `DIRECT` queue는 그래픽스 명령을 실행할 수 있다.
- Phase 2의 clear, Phase 3의 draw call 모두 이 queue에 제출된다.

제출 코드:

```cpp
ID3D12CommandList* commandLists[] = { m_commandList.Get() };
m_commandQueue->ExecuteCommandLists(1, commandLists);
```

## 3. Command Allocator

Command Allocator는 command list가 기록하는 명령 메모리를 제공한다.

현재 코드:

```cpp
m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  IID_PPV_ARGS(&m_commandAllocator));
```

Render에서 매 프레임:

```cpp
m_commandAllocator->Reset();
```

중요한 점:

> GPU가 아직 해당 allocator에 기록된 명령을 읽고 있으면 Reset하면 안 된다.

현재 Phase 2에서는 매 프레임 `WaitForGpu()`로 GPU 완료를 기다리기 때문에 allocator 하나를 안전하게 재사용한다.

나중에는 frame resource를 도입해 프레임마다 allocator를 따로 둔다.

## 4. Command List

Command List는 실제 GPU 명령을 기록하는 객체다.

생성:

```cpp
m_device->CreateCommandList(..., IID_PPV_ARGS(&m_commandList));
```

명령 기록 흐름:

```cpp
m_commandList->Reset(m_commandAllocator.Get(), nullptr);
m_commandList->ResourceBarrier(...);
m_commandList->ClearRenderTargetView(...);
m_commandList->ResourceBarrier(...);
m_commandList->Close();
```

의도:

- CPU가 GPU 명령을 기록한다.
- 기록이 끝나면 `Close()`한다.
- 닫힌 command list만 queue에 제출할 수 있다.

## 5. 왜 생성 직후 Close하는가?

D3D12에서 `CreateCommandList`로 만든 command list는 생성 직후 열린 상태다.

하지만 렌더 루프에서는 매 프레임 이렇게 쓰고 싶다.

```cpp
Reset()
기록
Close()
Execute
```

그러려면 초기 상태에서 이미 닫혀 있어야 한다.

그래서 초기화 단계에서 한 번 닫는다.

```cpp
return CheckHR(m_commandList->Close(), "Initial CommandList Close");
```

## 6. 현재 한 프레임 명령 순서

```text
CommandAllocator Reset
CommandList Reset
ResourceBarrier: Present -> RenderTarget
ClearRenderTargetView
ResourceBarrier: RenderTarget -> Present
CommandList Close
ExecuteCommandLists
Present
WaitForGpu
```

이 순서가 Phase 2 렌더링의 핵심이다.

## 7. 실수하기 쉬운 점

- 닫히지 않은 command list를 제출하면 안 된다.
- GPU가 아직 allocator를 쓰고 있는데 allocator를 Reset하면 안 된다.
- command list는 Reset 후 다시 명령을 기록해야 한다.
- queue에 제출했다고 GPU가 즉시 끝낸 것이 아니다.
- command list에 기록한 resource state와 실제 resource state가 맞아야 한다.

## 8. 앞으로 확장될 부분

Phase 3:

- Root Signature 설정
- Pipeline State 설정
- Viewport/Scissor 설정
- Vertex Buffer 바인딩
- DrawInstanced 기록

Phase 4:

- 프레임별 Command Allocator
- Command Queue 래퍼
- GPU/CPU 동기화 구조 개선

Phase 6 이후:

- Sprite Batch의 draw call들을 command list에 기록
