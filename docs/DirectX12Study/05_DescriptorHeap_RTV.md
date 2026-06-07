# Descriptor Heap, RTV

## 1. Descriptor란?

D3D12에서 descriptor는 GPU 리소스를 사용하는 방법을 설명하는 작은 핸들이다.

중요한 점:

> 리소스 자체와 descriptor는 다르다.

예를 들어 back buffer 리소스가 있어도, 그 리소스를 render target으로 쓰려면 RTV descriptor가 필요하다.

## 2. Descriptor Heap

Descriptor Heap은 descriptor들이 들어 있는 배열이다.

Phase 2에서는 RTV descriptor heap만 사용한다.

```cpp
D3D12_DESCRIPTOR_HEAP_TYPE_RTV
```

현재 코드 위치:

```text
D3D12Renderer::CreateRenderTargetViews()
```

생성 코드:

```cpp
D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
heapDesc.NumDescriptors = kFrameCount;

m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap));
```

## 3. RTV란?

RTV는 Render Target View의 약자다.

의미:

- 어떤 리소스를 render target으로 사용할지 설명한다.
- `ClearRenderTargetView`나 draw call의 출력 대상이 되려면 RTV가 필요하다.

현재 코드:

```cpp
m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
```

여기서 `m_renderTargets[i]`는 SwapChain에서 가져온 back buffer 리소스다.

## 4. RTV Handle 계산

Descriptor heap은 배열처럼 연속된 공간이다.

처음 handle:

```cpp
D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
```

다음 descriptor로 이동:

```cpp
rtvHandle.ptr += m_rtvDescriptorSize;
```

현재 프레임의 RTV를 선택:

```cpp
rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
```

의도:

- back buffer 0이면 RTV 0 사용
- back buffer 1이면 RTV 1 사용
- 현재 frame index와 RTV descriptor 위치를 맞춘다.

## 5. Descriptor Size

Descriptor 하나의 크기는 하드웨어/드라이버에 따라 다를 수 있다.

그래서 직접 `sizeof` 같은 걸 쓰면 안 된다.

현재 코드:

```cpp
m_rtvDescriptorSize =
    m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
```

이 값을 사용해 handle을 이동해야 한다.

## 6. 왜 RTV Heap은 Shader Visible이 아닌가?

RTV descriptor heap은 CPU가 command list에 render target을 지정할 때 쓰는 descriptor heap이다.

Phase 2에서는 셰이더가 리소스를 읽는 것이 아니므로 shader visible heap이 필요 없다.

나중에 텍스처를 셰이더에서 읽으려면 `CBV_SRV_UAV` descriptor heap을 만들고 shader visible로 설정해야 한다.

## 7. 실수하기 쉬운 점

- back buffer resource만 있고 RTV를 만들지 않으면 render target으로 clear할 수 없다.
- descriptor handle 이동은 `GetDescriptorHandleIncrementSize` 값을 써야 한다.
- frame index와 RTV index가 맞지 않으면 잘못된 back buffer를 대상으로 할 수 있다.
- ResizeBuffers 전에 기존 back buffer ComPtr를 해제해야 한다.
- ResizeBuffers 후에는 back buffer가 새로 생기므로 RTV도 다시 만들어야 한다.

## 8. 앞으로 확장될 부분

Phase 3:

- 렌더 타겟에 삼각형 draw 결과 출력

Phase 5:

- SRV descriptor heap 추가
- texture resource를 shader resource로 바인딩

Phase 6:

- sprite texture들을 descriptor로 관리

Phase 16:

- Dear ImGui D3D12 backend와 descriptor heap 공유/분리 전략 필요
