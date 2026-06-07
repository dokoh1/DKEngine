# DirectX12 Study

이 폴더는 DirectX 12 개념을 **Phase별이 아니라 기능별**로 정리한다.

Phase 문서는 "그 Phase에서 무엇을 만들었는가"를 기록하고, 이 폴더는 "그 기능이 DirectX 12에서
무슨 의미이며 왜 필요한가"를 누적해서 설명한다.

앞으로 Phase 3, Phase 5, Phase 6에서 새 개념이 나오면 관련 기능 문서에 내용을 추가하거나,
새 기능 문서를 만든다.

## Phase 2에서 사용한 개념

| 문서 | 다루는 내용 |
|------|-------------|
| [01_DebugLayer_DXGI.md](01_DebugLayer_DXGI.md) | D3D12 Debug Layer, DXGI Factory, Adapter 선택 |
| [02_Device_COM_ComPtr.md](02_Device_COM_ComPtr.md) | D3D12 Device, COM, `ComPtr`, `HRESULT` |
| [03_CommandSystem.md](03_CommandSystem.md) | Command Queue, Command Allocator, Command List |
| [04_SwapChain_BackBuffer_Present.md](04_SwapChain_BackBuffer_Present.md) | SwapChain, Back Buffer, Present, V-Sync |
| [05_DescriptorHeap_RTV.md](05_DescriptorHeap_RTV.md) | Descriptor, Descriptor Heap, RTV |
| [06_ResourceState_Barrier.md](06_ResourceState_Barrier.md) | Resource State, Resource Barrier |
| [07_Fence_Synchronization.md](07_Fence_Synchronization.md) | Fence, CPU/GPU 동기화, Event |
| [08_ResizeBuffers.md](08_ResizeBuffers.md) | 창 리사이즈, `ResizeBuffers`, RTV 재생성 |

## 현재 코드에서 보는 위치

Phase 2 D3D12 코드는 주로 아래 파일에 있다.

```text
src/RHI/D3D12Renderer.h
src/RHI/D3D12Renderer.cpp
```

`Application`은 렌더러를 호출만 한다.

```text
src/Core/Application.h
src/Core/Application.cpp
```

## 문서 작성 규칙

- 기능별로 작성한다.
- 새 Phase에서 기존 기능을 더 깊게 쓰면 기존 문서에 추가한다.
- 완전히 새로운 기능이면 새 문서를 만든다.
- 문서에는 반드시 다음을 포함한다.
  - 이 개념이 무엇인지
  - 왜 필요한지
  - 현재 코드에서 어디에 쓰였는지
  - 실수하기 쉬운 점
  - 앞으로 어느 Phase에서 확장될지
