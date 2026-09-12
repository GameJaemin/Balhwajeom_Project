---
title: "상호작용 시스템"
status: ReviewRequired
authority: Supporting
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Public API·설정·런타임 계약과 재대조하지 않음"
---

# 상호작용 시스템

## 목적

`UJMInteractionComponent`는 특정 Player Character나 입력 시스템에 의존하지 않고 라인트레이스 기반 상호작용을 실행하기 위한 컴포넌트입니다.

플레이어뿐 아니라 AI, NPC, 임의의 Actor도 이 컴포넌트를 통해 상호작용을 시도할 수 있습니다.

## 주요 타입

- `FJMInteractionContext`: 실행자, 컨트롤러, 대상 Actor, 상호작용 위치와 방향, Gameplay Tag를 담는 공통 Context
- `FJMInteractionPrompt`: 화면에 표시할 프롬프트 정보
- `FJMInteractionResult`: 성공 여부, 결과 코드, 메시지
- `IJMInteractorInterface`: 상호작용 실행자의 시점 위치, 방향, 태그를 제공하는 선택 인터페이스
- `IJMInteractableInterface`: 대상이 구현하는 상호작용 API
- `UJMInteractionComponent`: 라인트레이스 탐색과 상호작용 실행 담당

## 상호작용 대상 API

`IJMInteractableInterface`는 다음 개념을 제공합니다.

```text
CanInteract
BeginInteract
CompleteInteract
CancelInteract
GetInteractionPrompt
GetInteractionDuration
GetInteractionPriority
```

Blueprint에서 확장할 수 있도록 `BlueprintNativeEvent`로 선언되어 있습니다.

## Trace Mode

`UJMInteractionComponent`는 다음 탐색 방식을 지원합니다.

- `Manual`: 외부에서 직접 Refresh 호출
- `OnInput`: 입력 시점에 필요하면 Refresh
- `Timer`: 설정된 간격으로 Refresh
- `Tick`: 매 프레임 Refresh

기본값은 `Manual`입니다. 숨겨진 Tick 비용을 피하기 위해, 게임 쪽 입력 이벤트에서 명시적으로 호출하는 방식을 우선합니다.

## Trace 위치와 방향

상호작용 Trace는 다음 순서로 시점을 정합니다.

1. 실행 Actor가 `JMInteractorInterface`를 구현했다면 `GetInteractionView` 결과 사용
2. 실행 Actor가 Pawn이면 Controller의 `GetPlayerViewPoint` 사용
3. 실행 Actor가 Controller이면 Controller의 `GetPlayerViewPoint` 사용
4. 실행 Actor에 Camera Component가 있으면 Camera 위치와 Forward Vector 사용
5. 마지막으로 Actor 위치와 Forward Vector 사용

Character Blueprint에 Camera가 붙어 있는 일반적인 플레이어라면 화면 중앙 기준으로 Trace가 나갑니다.

## Trace Radius

`Trace Radius`가 `0`이면 기존 Line Trace입니다.

얇은 Static Mesh, 작은 아이템, Box Collision을 살짝 빗겨 보는 경우에는 `Trace Radius`를 `10 ~ 30` 정도로 설정해보세요. 그러면 선 하나가 아니라 작은 구를 앞으로 굴리는 방식으로 대상을 찾습니다.

처음 디버깅할 때 추천값:

```text
Trace Distance: 500
Trace Radius: 20
Trace Channel: Visibility
Draw Debug: true
```

## Hit Actor에서 조사 대상 찾는 순서

Trace가 무언가에 맞으면 다음 순서로 `JMInteractableInterface`를 구현한 대상 또는 컴포넌트를 찾습니다.

1. 맞은 Component의 Owner Actor
2. 맞은 Actor
3. 맞은 Actor의 Owner
4. 맞은 Actor의 Attach Parent

각 Actor에서는 먼저 Actor 자신이 `JMInteractableInterface`를 구현했는지 확인하고, 그 다음 Actor에 붙어 있는 Component 목록에서 구현체를 찾습니다.

일반적인 조사 아이템은 맞은 Blueprint Actor 안에 `UJMInspectableComponent`가 있으므로 바로 잡힙니다. Blueprint 안에 `ChildActorComponent`나 별도 충돌 Actor를 쓰는 구조에서는 Owner 또는 Attach Parent에 붙은 `UJMInspectableComponent`도 찾을 수 있습니다.

## 기본 Blueprint 사용법

상호작용을 실행할 Actor에 `UJMInteractionComponent`를 추가합니다.

입력 이벤트에서 다음 순서로 호출합니다.

```text
Refresh Current Interactable
Try Begin Interaction
```

Timer나 Tick 모드를 사용한다면 입력 이벤트에서는 `Try Begin Interaction`만 호출해도 됩니다.

## 기본 C++ 사용법

```cpp
if (UJMInteractionComponent* InteractionComponent = FindComponentByClass<UJMInteractionComponent>())
{
    InteractionComponent->RefreshCurrentInteractable();
    const FJMInteractionResult Result = InteractionComponent->TryBeginInteraction();
}
```

## 디버그

Project Settings 또는 컴포넌트 설정에서 Debug Draw를 켜면 라인트레이스 선과 충돌 지점을 확인할 수 있습니다.

`Trace Radius`가 0보다 크면 시작점, 끝점, 충돌 지점에 구가 같이 그려집니다.

## 주의사항

- 특정 Player Character로 Cast하지 않습니다.
- `GetActorOfClass` 반복 호출을 사용하지 않습니다.
- 입력 바인딩은 게임 프로젝트가 담당합니다.
- 상호작용 대상이 도중에 파괴되면 `TWeakObjectPtr` 기반 참조가 안전하게 무효화됩니다.
- 조사 대상 Blueprint의 Static Mesh 또는 Collision Component가 `Trace Channel`을 Block해야 합니다.
