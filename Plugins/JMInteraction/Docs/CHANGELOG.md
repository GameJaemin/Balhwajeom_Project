---
title: "Changelog"
status: ReviewRequired
authority: Historical
scope: "Plugin: JMInteraction"
classified_on: 2026-08-19
owners:
  - "JMInteraction Maintainers"
review_reason: "릴리스 및 Git 이력과 변경 항목의 누락 여부를 재대조하지 않음"
---

# Changelog

## Unreleased

- Track nested modal depth so closing an Item Inspector above Inventory does not restore the interaction prompt early.

## 1.1.1 - 2026-07-23

- 공통 `Event.UI.Modal.*` 이벤트를 구독해 Inventory 같은 Modal UI 동안 Prompt를
  자동 숨김하고 종료 시 이전 Suppression 상태 복원

## 1.1.0 - 2026-07-21

- Modal UI용 `SetPromptSuppressed`/`IsPromptSuppressed` API 추가
- Item Inspector가 열려 있는 동안 Interaction Prompt 자동 숨김 및 종료 시 이전 상태 복원

### Fixed

- 기본 Manual Trace 때문에 최초 Prompt가 E 입력까지 지연되던 문제
- 기본 Prompt가 화면 중앙 Dot과 겹치던 배치 문제
- 파괴된 CurrentInteractable의 weak reference가 만료된 뒤 Prompt가 남을 수 있던 문제

### Added

- 중앙 기준 Screen Offset, Font Size, Render Scale, Padding, 정렬, Hidden Mode 설정
- Project Settings 전역 스타일과 Component별 스타일 Override
- Prompt 값 캐시 및 변경 시에만 Text/Visibility 갱신
- 표시/숨김/텍스트/스타일 Blueprint 이벤트와 Development 디버그 로그
- Widget Blueprint Designer용 선택적 `PromptText`/`PromptContainer` 자동 바인딩
- Native fallback 이후 Designer 미리보기가 `Collapsed`로 남을 수 있던 문제 수정
- 배경 없는 커스텀 Prompt Widget 제작 가이드

### Changed

- 기본 Trace Mode를 0.1초 Timer로 변경
- 상호작용 완료 후 즉시 재탐색하여 Door Prompt 변경 및 Item Prompt 숨김
- CanInteract가 false인 대상도 Focus/Prompt 계약을 평가할 수 있도록 감지와 실행 가능 여부를 분리

## 1.0.0 - 2026-07-21

### Added

- 독립 Runtime `JMInteraction` 모듈
- CenterScreenTrace / MouseCursorTrace
- Manual / OnInput / Timer / Tick 갱신
- Focus begin/end, Prompt delegate, 기본/교체형 Widget
- Blueprint용 `UJMInteractableComponent`
- Project Settings와 Development-only debug trace

### Changed

- 공통 interaction 계약을 ItemInspectorRuntime에서 JMInteraction으로 추출
- Inventory, Dialogue, Door integration의 의존 방향 전환
- 모든 `AInventoryWorldItemPickup`에 기본 상호작용 진입점 추가

### Compatibility

- 기존 Blueprint와 include를 위한 Core Redirect 및 forwarding header 제공
