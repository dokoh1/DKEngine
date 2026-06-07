# Resource State, Resource Barrier

## 1. Resource State란?

D3D12 리소스는 현재 어떤 용도로 쓰이는지 상태를 가진다.

예:

- `D3D12_RESOURCE_STATE_PRESENT`
- `D3D12_RESOURCE_STATE_RENDER_TARGET`
- `D3D12_RESOURCE_STATE_COPY_DEST`
- `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE`

D3D12는 리소스를 사용할 때 상태 전이를 개발자가 명시해야 한다.

D3D11에서는 드라이버가 많은 부분을 자동으로 처리했지만, D3D12는 개발자가 직접 기록한다.

## 2. Phase 2에서 필요한 상태

Phase 2의 back buffer는 두 가지 상태만 오간다.

```text
Present
RenderTarget
```

프레임 시작 시:

```text
Present 상태
```

Clear하려면:

```text
RenderTarget 상태
```

Present하려면 다시:

```text
Present 상태
```

## 3. Resource Barrier란?

Resource Barrier는 리소스 상태 전이를 GPU command list에 기록하는 명령이다.

현재 코드 위치:

```text
D3D12Renderer::Render()
```

Present -> RenderTarget:

```cpp
D3D12_RESOURCE_BARRIER toRenderTarget = {};
toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
toRenderTarget.Transition.pResource = m_renderTargets[m_frameIndex].Get();
toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
m_commandList->ResourceBarrier(1, &toRenderTarget);
```

RenderTarget -> Present:

```cpp
D3D12_RESOURCE_BARRIER toPresent = {};
toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
toPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
m_commandList->ResourceBarrier(1, &toPresent);
```

## 4. 왜 필요한가?

GPU 메모리의 같은 리소스를 여러 용도로 쓸 수 있다.

Back buffer는:

- 화면 표시용으로 쓰일 수 있고
- render target으로 쓰일 수도 있다.

하지만 GPU 입장에서는 두 용도가 다르다.

D3D12는 이런 전환을 명시해야 GPU가 캐시, 메모리 접근, 실행 순서를 올바르게 관리할 수 있다.

## 5. 현재 한 프레임 상태 흐름

```text
현재 back buffer: PRESENT
    |
    | ResourceBarrier
    v
RENDER_TARGET
    |
    | ClearRenderTargetView
    v
RENDER_TARGET
    |
    | ResourceBarrier
    v
PRESENT
    |
    | SwapChain Present
    v
화면 표시
```

## 6. Debug Layer와의 관계

Resource Barrier가 틀리면 D3D12 Debug Layer가 경고를 낼 수 있다.

예:

- 실제 상태는 Present인데 RenderTarget이라고 잘못 가정
- RenderTarget 상태로 돌리지 않고 Present 호출
- 잘못된 리소스에 barrier 적용

그래서 Phase 2부터 Debug Layer를 켠다.

## 7. 실수하기 쉬운 점

- `StateBefore`와 실제 리소스 상태가 맞아야 한다.
- Present 전에 반드시 `D3D12_RESOURCE_STATE_PRESENT`로 되돌려야 한다.
- 여러 back buffer가 있으면 현재 frame index의 리소스에 barrier를 걸어야 한다.
- Resize 후 새 back buffer는 다시 상태 흐름을 맞춰야 한다.
- Barrier는 CPU에서 즉시 상태를 바꾸는 것이 아니라 command list에 GPU 명령으로 기록된다.

## 8. 앞으로 확장될 부분

Phase 3:

- Vertex Buffer는 `VERTEX_AND_CONSTANT_BUFFER` 상태로 사용

Phase 5:

- Texture upload 시 `COPY_DEST -> PIXEL_SHADER_RESOURCE`
- Upload buffer와 default heap texture 사이 복사

Phase 6:

- Sprite texture들을 shader resource 상태로 사용

Phase 16:

- Render target과 ImGui draw 사이 상태 관리
