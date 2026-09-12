---
title: "변경 기록"
status: ReviewRequired
authority: Historical
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "릴리스 및 Git 이력과 변경 항목의 누락 여부를 재대조하지 않음"
---

# 변경 기록

## Unreleased

- Inspection sessions now track their owning World, cancel CoreTicker transitions during matching World cleanup, and revalidate Widget/session state after open delegates.
- Empty inspect interaction text is now respected, allowing designers to hide a prompt intentionally.
- Simple UI transitions now keep the Inspector backdrop opaque and animate only `MainRow`/`ContentRow`, avoiding background flicker between modal screens.
- Added an optional full-widget fade transition for Inspector sessions opened from another UI such as Inventory.
- Fixed preview dragging leaving mouse capture active when the pointer exits the preview, which could prevent the Inspector close button from receiving later clicks.
- Preview capture is now released on pointer exit, lost mouse-up, input disable, close request, and widget destruction.
- Item Inspector now publishes the shared `Event.UI.Modal.*` lifecycle so other HUD widgets hide while inspection is open.
- Nested modal UI (for example Inventory -> Item Inspector) keeps the HUD suppressed until the final modal closes.

- Item Inspector 진입 시 `JMInteractionComponent` Prompt를 자동으로 숨기고 종료 완료 시 이전 suppression 상태를 복원한다.
- 원본 Static Mesh가 유지되는 종이/열쇠 검사 화면에서 상호작용 문구가 남는 문제를 수정했다.

## 0.5.0

- Added an optional exit transition that reverses the preview image and preview pose back to a valid world source actor.
- The inspector remains open in `TransitioningOut` until the reverse animation finishes; `OnInspectionClosed` is broadcast afterward.
- World sources are revealed during the final handoff and retain their original hidden state on cleanup.
- Inventory UI inspections still close immediately because they have no source actor.
- Inspect-before-pickup integrations explicitly suppress the exit transition so pickups do not visually return before collection.
- World teardown, failed opens, replacements, invalid sources, and unavailable layouts keep the immediate cleanup path.

## 0.4.0

- Fixed entrance transitions being skipped when the inspector preview panel had no cached geometry during its first viewport frame.
- Source bounds projection now tolerates individual corners crossing behind the camera instead of cancelling the entire transition.
- Added an optional source-actor-to-inspector entrance transition.
- The transition projects source bounds into viewport space and moves a transient Render Target image into the inspector preview panel.
- Preview rotation, offset, scale, and zoom interpolate toward `ViewSettings`; completion reapplies the authored Data Asset values exactly.
- Added project defaults, per-Data-Asset overrides, and per-request overrides through `FJMItemInspectionTransitionSettings`.
- Added the `TransitioningIn` state and pause-independent Core Ticker lifetime management.
- Source actors are restored on close, failure, cancellation, and world teardown.
- Invalid or off-screen sources safely use the previous immediate-open path.

## 0.3.0

- Preview Mesh와 같은 Pivot에서 회전·확대되는 선택형 World-space UMG 표면 콘텐츠 추가
- `UJMItemInspectionSurfaceWidgetBase`와 네이티브 기본 TextBlock fallback 추가
- `UJMItemInspectionData.SurfaceWidget`에 Widget Class, 본문, 해상도, Transform, 양면 설정 추가
- `Description`을 표면 본문으로 자동 재사용하는 제작 흐름 추가
- 기존 `WB_NoteMaster`, `DA_PaperInspection`, `SM_Paper` 샘플 연결 및 회전 활성화
- 실제 Paper 에셋의 위젯 생성, 본문 전달, 공통 Pivot 회전을 검증하는 `JM.ItemInspector.SurfaceWidget.PaperPreview` 자동화 테스트 추가

## 0.2.0

- 플러그인 이름을 `JMGameplayFramework`에서 `ItemInspector`로 변경
- Runtime 모듈 이름을 `JMGameplayRuntime`에서 `ItemInspectorRuntime`으로 변경
- 기존 플러그인 콘텐츠와 설정을 위한 Core Redirect 추가

## 0.1.1

- 프로젝트 `Content/Item/WBP_JMItemInspection` 위젯을 플러그인 콘텐츠 `/ItemInspector/UI/WBP_JMItemInspection`으로 복제
- 새 프로젝트에서 별도 위젯 지정 없이 플러그인 기본 Inspector Widget을 우선 사용하도록 기본값 변경
- 설정된 Widget Blueprint 로드에 실패하면 기본 C++ Inspector Widget으로 fallback하도록 보강
- Inspector가 이미 열려 있을 때 중복 열기 실패가 기존 열린 상태를 `Failed`로 바꾸던 문제 수정
- Blueprint 상호작용 안정성을 위해 `Trace Radius` 기반 Sphere Trace 옵션 추가
- Character Blueprint에서 Controller/Camera 시점 기준으로 상호작용 Trace가 나가도록 보강
- Child Actor 또는 별도 충돌 Actor 구조에서 Owner/Attach Parent의 조사 컴포넌트도 찾도록 보강
- 다른 프로젝트 이식을 위한 `PORTABLE_INSTALL.md` 문서 추가

## 0.1.0

- `ItemInspector` Runtime Plugin 생성
- `ItemInspectorRuntime` 모듈 생성
- Core 로그, 설정, 상호작용 공통 타입 추가
- `IJMInteractorInterface` 추가
- `IJMInteractableInterface` 추가
- `UJMInteractionComponent` 라인트레이스 상호작용 기반 구현
- `UJMItemInspectionData` 추가
- `UJMInspectableComponent` 추가
- `UJMItemInspectionSubsystem` 추가
- `UJMItemInspectionWidgetBase` 기본 위젯 추가
- 한국어 문서와 사용 안내 README 추가
- 기본 Inspector 위젯에 이름, 분류, 설명, 추가 정보, Item ID 표시 추가
- 기본 Inspector 위젯의 ESC 및 `닫기 (ESC)` 닫기 흐름 보강
- Runtime Preview Actor 추가
- SceneCapture2D와 Render Target 기반 Static Mesh Preview 추가
- Preview 영역 마우스 드래그 회전 추가
- Preview 영역 마우스 휠 줌 추가
- 커스텀 Inspector Widget 제작/이식 가이드 문서 추가
- Preview 조명을 Directional Light에서 Spot/Point 기반 Key/Fill/Rim 구성으로 변경
- `Multiple directional lights are competing...` 경고를 피하도록 Preview 전용 Directional Light 제거
- `UJMItemInspectionData.ViewSettings.LightingSettings` 추가
- Preview 조명 프리셋, 커스텀 조명 강도/색상, 그림자, Lighting Channel 격리 옵션 추가
