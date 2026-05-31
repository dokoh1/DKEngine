# DKEngine 개발 노트 (docs)

각 Phase에서 **무엇을 만들었고, 왜 그렇게 했는지(의도)** 를 기록하는 폴더다.
코드만 보면 "어떻게"는 알아도 "왜"는 사라지기 쉽다. 이 노트는 그 "왜"를 남겨,
나중에 다시 읽었을 때 설계 의도와 학습 포인트를 복기할 수 있게 한다.

| Phase | 문서 | 한 줄 요약 |
|------|------|-----------|
| 0 | [Phase00_Setup.md](Phase00_Setup.md) | 개발 환경·빌드 시스템·프로젝트 구조·git 세팅과 그 이유 (MSVC/SDK, VS솔루션/CMake 설명 포함) |
| 1 | [Phase01_Win32Window.md](Phase01_Win32Window.md) | Win32 창 + 게임 루프. WndProc-객체 연결, PeekMessage 루프 해설 |

## 개념 정리 ([concepts/](concepts/README.md))
특정 Phase에 묶이지 않는 일반 개발 지식.

| # | 문서 | 한 줄 요약 |
|---|------|-----------|
| 01 | [concepts/01_Compile_and_Build.md](concepts/01_Compile_and_Build.md) | 컴파일 vs 빌드의 차이, 전처리→컴파일→링크 전체 흐름 |

> 전체 로드맵은 상위 폴더의 [`ROADMAP.md`](../ROADMAP.md), 빌드/실행은 [`QUICKSTART.md`](../QUICKSTART.md) 참고.

## 노트 작성 형식
각 Phase 문서는 다음 흐름을 따른다:
1. **이번 Phase의 목표** — 무엇을 끝내려 했나
2. **무엇을 만들었나** — 추가/변경된 파일과 역할
3. **왜 이렇게 했나 (의도)** — 설계 결정과 대안, 트레이드오프
4. **코드 깊이 읽기** — 핵심 코드 조각을 한 줄씩 해설
5. **여기서 배우는 지식** — 로드맵의 "선행 지식"이 코드로 어떻게 나타나는가
6. **완료 기준 / 다음 단계**
