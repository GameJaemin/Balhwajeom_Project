---
title: "Architecture"
status: ReviewRequired
authority: Canonical
scope: "Plugin: JMInteraction"
classified_on: 2026-08-19
owners:
  - "JMInteraction Maintainers"
review_reason: "2026-08-19 전체 감사 이후 문서 구조만 정비했으며 모든 세부를 현재 코드와 재대조하지 않음"
---

# Architecture

## 책임

- `UJMInteractionComponent`: 탐색, 거리/인터페이스 검증, 현재/활성 대상의 weak reference, Focus 전환, Prompt 갱신, 실행
- `IJMInteractableInterface`: 대상의 가능 여부, Prompt, Begin/Complete/Cancel, Focus 계약
- `IJMInteractorInterface`: 선택적 시점 및 Interactor Gameplay Tag 공급
- `UJMInteractableComponent`: C++ 없이 조합하는 Blueprint용 기본 구현
- `UJMInteractionSettings`: Project Settings 기본값
- `UJMInteractionPromptWidgetBase`: 교체 가능한 로컬 Prompt presenter

비책임은 Door 상태 변경, Inventory 추가, Dialogue 재생, 입력 매핑, 커서 모드 변경이다.

## 데이터 흐름

```text
Timer/Tick/Manual refresh
  -> Center trace 또는 cursor hit
  -> 거리 검사 + Actor/ActorComponent의 IJMInteractableInterface 탐색
  -> 이전 OnFocusEnd / 새 OnFocusBegin
  -> Prompt 캐시 비교 + delegate + Widget

Player Input
  -> CanInteract
  -> TryInteract
  -> BeginInteract
  -> 즉시 또는 Hold 후 CompleteInteract
  -> 대상 플러그인의 실제 행동
```

감지 단계는 `CanInteract`가 false여도 대상을 유지할 수 있다. 이 경우 대상이 `GetInteractionPrompt`에서 실패 이유와 `bVisible=true`를 반환하면 입력 전에 이유를 표시할 수 있고, 실제 실행은 `TryInteract`의 `CanInteract` 검사에서 차단된다.

대상 참조는 `TWeakObjectPtr`로 보관한다. 파괴된 대상은 다음 갱신 또는 실행 시 안전하게 실패한다. UI는 Local Controller에서만 생성되고 Dedicated Server에서는 비활성화된다.

## 의존성과 마이그레이션

기존 공통 타입은 `ItemInspectorRuntime`에서 추출했다. include 호환을 위해 `Interaction/...` 및 `Core/JMGameplayTypes.h` forwarding 경로를 유지하고, 기존 Blueprint 클래스/구조체/열거형 경로는 `Config/DefaultJMInteraction.ini`의 Core Redirect로 이동한다.

현재 소비 방향:

- `ItemInspectorRuntime -> JMInteraction`
- `InventorySystem -> JMInteraction` (조사 연동 때문에 ItemInspector도 별도 유지)
- `ReusableDialogueSystem -> JMInteraction`
- `JMDoorGameplayIntegration -> JMInteraction`

네트워크 권한 확정은 각 도메인 대상의 책임이다. JMInteraction 자체는 로컬 탐색/요청 계층이며 멀티플레이 권한을 보장한다고 주장하지 않는다. 저장 데이터는 소유하지 않는다.
