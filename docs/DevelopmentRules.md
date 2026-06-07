# DKEngine 개발 규칙

이 문서는 Phase를 진행할 때 반복해서 지킬 작업 기준이다.
새 기능을 구현할 때 코드, 문서, 검증이 같은 기준으로 남도록 한다.

## 1. Phase 주석 규칙

- 새 Phase에서 작성한 코드에는 `[Phase N]` 주석을 붙인다.
- 기존 코드를 수정한 경우에도 수정된 함수, 멤버, 핵심 블록에 `[Phase N]` 표시를 남긴다.
- 모든 줄마다 붙이지 않는다. 파일, 클래스, 함수, 멤버, 중요한 로직 블록 단위로 표시한다.
- 이미 이전 Phase에서 작성된 코드에는 기존 Phase 표시를 유지한다.
- 새 Phase가 기존 코드를 확장한 경우에는 두 Phase를 함께 표시할 수 있다.

예:

```cpp
// [Phase 1] Win32 창 생성과 메시지 처리.
Window m_window;

// [Phase 2] D3D12 디바이스/스왑체인/펜스를 소유하는 렌더러.
D3D12Renderer m_renderer;
```

## 2. 코드 구조 규칙

- `Application`은 실행 흐름을 조율하고, 세부 구현은 하위 모듈에 둔다.
- Win32 세부 구현은 `src/Core/Window`에 둔다.
- D3D12 세부 구현은 `src/RHI/`에 둔다.
- Phase 4 전까지는 D3D12 코드를 과하게 세분화하지 않는다. 단, `Application.cpp`에 D3D12 COM 객체를 직접 쌓지 않는다.
- OS 핸들, COM 객체, GPU 리소스는 만든 객체가 정리 책임도 갖는다.
- 가능하면 raw `new/delete` 대신 값 멤버, `std::unique_ptr`, `Microsoft::WRL::ComPtr`를 사용한다.

## 3. 문서 갱신 규칙

Phase를 진행하면 관련 문서를 함께 갱신한다.

- 새 Phase 문서: `docs/PhaseXX_*.md`
- 전체 진행 상태: `ROADMAP.md`
- 실행/빌드 안내: `QUICKSTART.md`
- 문서 목록: `docs/README.md`
- 아키텍처 변경이 있으면: `docs/Architecture.md`
- DirectX 12 개념이 새로 나오거나 확장되면: `docs/DirectX12Study/`

Phase 문서에는 최소한 다음을 적는다.

1. 이번 Phase의 목표
2. 무엇을 만들었나
3. 왜 이렇게 했나
4. 핵심 코드 흐름
5. 여기서 배우는 지식
6. 완료 기준 / 다음 단계

DirectX 12 개념 문서는 Phase별로 만들지 않고 기능별로 만든다.
예를 들어 SwapChain, Fence, Resource Barrier처럼 개념 단위로 문서를 유지한다.

## 4. 검증 규칙

Phase 작업이 끝나기 전에 다음을 확인한다.

- `Debug|x64` 빌드
- 가능하면 `Release|x64` 빌드
- `git diff --check`
- 실행 검증이 필요한 Phase는 실제 실행 확인
- D3D12 렌더링 Phase는 가능하면 PIX GPU Capture 확인

검증 결과는 최종 응답에 짧게 남긴다.

## 5. D3D12 작업 규칙

- Debug 빌드에서는 D3D12 Debug Layer를 켠다.
- D3D12 Debug Layer 경고는 가능한 한 0으로 유지한다.
- 리소스 상태 전이(Resource Barrier)는 명시적으로 작성하고, 왜 필요한지 주석으로 남긴다.
- GPU가 사용할 수 있는 리소스를 해제하거나 리사이즈하기 전에는 Fence로 완료를 보장한다.
- PIX에서 보기 쉽도록 주요 D3D12 객체에는 `SetName`을 붙인다.

## 6. 완료 보고 규칙

작업 완료 시 다음을 간단히 보고한다.

- 어떤 파일을 바꿨는지
- 어떤 기능이나 구조가 추가됐는지
- 어떤 문서를 갱신했는지
- 어떤 검증을 통과했는지
- 남은 미확인 사항이 있으면 명시한다
