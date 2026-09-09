# 상호작용 시스템 담당 작업 문서

> 기준 문서: [Project-Progress-and-Roadmap.md](../Project-Progress-and-Roadmap.md) 2.3(증거 상호작용), 5장 P0-3
> 담당 범위: `UPlayerInteractionComponent`, `UInspectionComponent`, `ABalhwajeomEvidenceActor`의 F 상호작용/거리 표시 부분

이 문서는 **상호작용 세부 시스템 수정**을 맡은 담당자용 문서다. 거리 판정, F 조사, 상태 전환 흐름은 프로토타입 완료 상태이며, 이번 일정에서는 본편 증거 오브젝트로 확장하면서 드러나는 세부 버그와 UX를 다듬는 것이 핵심이다.

## 1. 현재 구조 (변경 대상 아님)

핵심 파일: [PlayerInteractionComponent.h](../../Source/Balhwajeom/Public/Interaction/PlayerInteractionComponent.h) / `.cpp`, [PlayerInteractionTypes.h](../../Source/Balhwajeom/Public/Interaction/PlayerInteractionTypes.h), [BalhwajeomEvidenceActor.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceActor.h)

동작 개요:
- `UPlayerInteractionComponent`가 매 틱 `RefreshInspectableObjects`로 주변 `UInspectionComponent`를 스캔하고, `ClassifyDistanceBetweenPoints`(Close/Middle/Max 거리 임계값)로 거리 상태(`EPlayerInspectionDistanceState`)를 판정한다.
- `CanInspectDistanceState`로 F 조사 가능 여부를 걸러내고, `TryInspect`/`RequestInspect`가 실제 상호작용을 실행한다.
- `ABalhwajeomEvidenceActor::RequestInvestigationInteraction`이 `UBalhwajeomInvestigationSubsystem::BeginEvidenceInteraction`/`CompleteEvidenceInteraction`을 호출해 `Once`, `Repeatable`, `ChangeState`, 단순 텍스트, 키워드 선택 창 흐름을 처리한다.
- 거리 라벨은 `SetInspectionLabel`/`ApplyInspectionDistanceState`로 갱신되며, 사진 카메라 HUD가 활성화되면 `SetInspectionLabelSuppressed`로 숨겨진다.

## 2. 변경 필요 사항 (세부 시스템 수정)

- [ ] **본편 증거 Blueprint 이전**: 기존/샘플 증거 Blueprint를 현재 `ABalhwajeomEvidenceActor` 구조(ObjectID/CurrentStateID/InspectionComponent 기반)로 옮긴다. 프로토타입 전용 하드코딩이 있으면 제거.
- [ ] Level_Main에 배치되는 실제 증거 오브젝트 수 기준으로 `RefreshInspectableObjects`의 스캔 비용(틱마다 전체 재스캔 여부)을 점검하고 필요 시 최적화.
- [ ] 거리별 라벨 문구(`FarLabel` 등, 로드맵 P2)의 실제 표시 규칙을 확정 — 최소한 이번 일정에서는 본편 증거 데이터 기준으로 라벨이 깨지지 않는지 확인.
- [ ] 카메라 모드/태블릿 오픈 등 다른 UI가 떠 있을 때 F 상호작용이 잘못 트리거되지 않는지 상태 전환 경계 케이스 점검.
- [ ] 여러 증거가 겹친 위치(`ResolveFocusedInspection`)에서 포커스 우선순위가 의도대로 동작하는지 본편 레벨 배치 기준으로 재검증.
- [ ] 잘못된 조합·이미 조사함·거리 부족 등 사용자 피드백 문구/연출을 통일(아트·이펙트·사운드 트랙과 인터페이스 공유, 로드맵 P2).

## 3. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: 본편 증거 Blueprint 이전, Level_Main 배치 기준 거리·F조사 1차 검증
- **2단계(9/13~9/15, M2)**: 겹침/우선순위 경계 케이스 수정, 실패 피드백 문구 통일(이펙트·사운드 연동 지점 확정)
- **3단계(9/16~9/17, M3)**: 전체 조사 루프(메신저→조사→촬영→태블릿) 통합 후 발견되는 상호작용 버그 수정
- **4단계(9/18, M4)**: 최종 QA 대응

## 4. 완료 조건

- 에디터를 새로 실행해 시작 맵부터 진술 반증까지 막힘 없이 1회 진행 가능(로드맵 P0-3 완료 조건 공유).
- 본편 레벨에 배치된 증거 오브젝트 전체가 프로토타입 전용 하드코딩 없이 동작한다.
- 거리 라벨·F 상호작용 관련 치명 오류 0건.

## 5. 주의 사항

- `.umap`, `BP_Evidence_*`는 월드/레벨 통합 작업과 겹치는 영역이므로 동시 편집 전 확인한다.
- 증거 상태는 이 트랙에서 별도로 캐싱하지 않고 항상 [Subsystem-Handoff.md](./Subsystem-Handoff.md)의 InvestigationSubsystem을 단일 기준으로 사용한다.
