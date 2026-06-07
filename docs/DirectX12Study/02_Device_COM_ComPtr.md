# Device, COM, ComPtr, HRESULT

## 1. D3D12 Device란?

D3D12 Device는 Direct3D 12에서 GPU 리소스와 여러 D3D12 객체를 만드는 중심 객체다.

Device가 만드는 대표 객체:

- Command Queue
- Command Allocator
- Command List
- Descriptor Heap
- Fence
- Buffer / Texture Resource
- Root Signature
- Pipeline State Object

Phase 2에서는 아직 Buffer, Texture, Shader는 만들지 않는다. 하지만 화면을 지우기 위해서도
Command Queue, Command List, RTV Descriptor Heap, Fence가 필요하므로 Device가 반드시 필요하다.

## 2. 현재 코드에서 쓰인 위치

```text
src/RHI/D3D12Renderer.cpp
└─ D3D12Renderer::CreateDevice()
```

현재 흐름:

```cpp
D3D12CreateDevice(selectedAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
```

의도:

- 선택한 하드웨어 adapter로 D3D12 Device를 만든다.
- Feature Level 11_0 이상이면 현재 Phase 2 작업에는 충분하다.
- Device에 이름을 붙여 PIX나 Debug Layer에서 알아보기 쉽게 한다.

```cpp
m_device->SetName(L"DKEngine D3D12 Device");
```

## 3. COM이란?

D3D12 객체들은 대부분 COM 객체다.

COM 객체는 일반 C++ 객체처럼 `delete`로 지우지 않는다. 내부적으로 참조 카운트를 가지고 있고,
참조가 0이 되면 해제된다.

대표 COM 인터페이스:

```cpp
ID3D12Device
ID3D12CommandQueue
ID3D12GraphicsCommandList
IDXGIFactory4
IDXGISwapChain3
```

COM의 특징:

- 인터페이스 포인터로 사용한다.
- `AddRef`, `Release` 기반 참조 카운트를 사용한다.
- 함수 결과를 `HRESULT`로 반환하는 경우가 많다.
- 인터페이스 변환은 `QueryInterface` 계열로 한다.

## 4. ComPtr를 쓰는 이유

`Microsoft::WRL::ComPtr<T>`는 COM 객체 수명을 자동으로 관리하는 스마트 포인터다.

현재 코드 예:

```cpp
Microsoft::WRL::ComPtr<ID3D12Device> m_device;
Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
```

의도:

- `Release()`를 직접 호출하지 않아도 된다.
- 함수가 실패하거나 중간에 return해도 누수 가능성이 줄어든다.
- `Reset()`으로 명시 해제가 가능하다.
- `.Get()`으로 raw pointer를 API에 넘길 수 있다.
- `IID_PPV_ARGS(&m_device)` 패턴과 잘 맞는다.

## 5. HRESULT

D3D12/DXGI 함수는 성공/실패를 `HRESULT`로 돌려주는 경우가 많다.

현재 코드에서는 `CheckHR` 헬퍼를 사용한다.

```text
src/RHI/D3D12Renderer.cpp
└─ CheckHR()
```

```cpp
if (SUCCEEDED(hr))
    return true;

DK_WARN("%s 실패 (HRESULT=0x%08X)", what, static_cast<unsigned int>(hr));
return false;
```

의도:

- 어떤 D3D12/DXGI 호출이 실패했는지 로그로 남긴다.
- 실패하면 초기화나 렌더링을 중단한다.
- Phase 2에서는 예외보다 bool 반환으로 흐름을 단순하게 유지한다.

## 6. IID_PPV_ARGS

D3D12 객체 생성 함수는 보통 "어떤 인터페이스를 받을지"와 "그 포인터를 어디에 쓸지"를 함께 요구한다.

예:

```cpp
m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
```

`IID_PPV_ARGS`는 다음을 안전하게 만들어준다.

- 요청할 COM 인터페이스 ID
- 결과 포인터 주소

직접 캐스팅하는 것보다 타입 실수 가능성이 줄어든다.

## 7. 실수하기 쉬운 점

- COM 객체를 raw pointer로 들고 있다가 `Release()`를 빼먹으면 누수된다.
- GPU가 아직 쓰는 리소스를 `Reset()`하면 Debug Layer 경고나 크래시가 날 수 있다.
- `HRESULT`를 무시하면 실패 원인을 놓치기 쉽다.
- `ComPtr::Get()`은 소유권을 넘기는 것이 아니다. 단순히 내부 raw pointer를 보여준다.
- `ComPtr::Reset()`은 참조를 해제한다. GPU 사용 완료 여부는 별도로 Fence로 보장해야 한다.

## 8. 앞으로 확장될 부분

Phase 3:

- Shader Blob
- Root Signature
- Pipeline State Object
- Vertex Buffer Resource

Phase 5:

- Texture Resource
- Upload Buffer
- SRV Descriptor

Phase 6 이후:

- 동적 버퍼
- 프레임 리소스
- descriptor allocator
