---
title: "Changelog"
status: ReviewRequired
authority: Historical
scope: "Plugin: JMGameplayEvent"
classified_on: 2026-08-19
owners:
  - "JMGameplayEvent Maintainers"
review_reason: "릴리스 및 Git 이력과 변경 항목의 누락 여부를 재대조하지 않음"
---

# Changelog

## Unreleased

### Changed

- Project Settings의 `JM Gameplay Event` 섹션을 `Plugins`에서 공통 `JM Plugins` 카테고리로 이동

### Added

- 기능 플러그인과 HUD가 구체 클래스를 참조하지 않고 협업하는
  `Event.UI.Modal.Opened` / `Event.UI.Modal.Closed` 공통 태그

## 1.0.0

### Added

- GameInstance-scoped Gameplay Tag event subsystem
- Exact and IncludeChildren subscriptions with GUID handles
- Weak Listener cleanup and mutation-safe synchronous dispatch
- Blueprint publish library and lifecycle-managed Listener Component
- Blueprintable UObject Payload base and Developer Settings
- Automation coverage for matching, unsubscribe, duplicate policy, mutation, nesting, and destroyed listeners
- Door, Inventory, Dialogue, and Interaction publisher migration
