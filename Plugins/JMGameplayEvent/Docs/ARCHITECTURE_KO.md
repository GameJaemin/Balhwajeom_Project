---
title: "JMGameplayEvent 아키텍처"
status: ReviewRequired
authority: Canonical
scope: "Plugin: JMGameplayEvent"
classified_on: 2026-08-19
owners:
  - "JMGameplayEvent Maintainers"
review_reason: "2026-08-19 전체 감사 이후 문서 구조만 정비했으며 모든 세부를 현재 코드와 재대조하지 않음"
---

# JMGameplayEvent 아키텍처

## 기존 구조 분석

- 범용 전역 이벤트 버스 또는 Gameplay Message Router 사용은 없었다.
- Gameplay Tags는 모든 기존 기능에서 사용 중이며 JMDoor는 Native Gameplay Tags를 이미 사용했다.
- Door, Inventory, Dialogue, Interaction은 각각 BlueprintAssignable Delegate를 제공했다.
- Dialogue와 ItemInspector는 `UGameInstanceSubsystem`/`ULocalPlayerSubsystem`, Door-Interaction 통합은 `UWorldSubsystem`을 사용한다.
- JMDoor 핵심은 외부 기능을 직접 호출하지 않고, 선택 통합 플러그인이 Interaction/Inventory를 직접 조합한다.
- Inventory는 ItemInspector와 JMInteraction, Dialogue는 JMInteraction을 직접 소비한다. 이들은 명령/기능 실행 관계이며 이번 Migration에서 제거하지 않았다.

## 수명과 격리

버스는 `UGameInstanceSubsystem`이다. 레벨 전환을 넘어 같은 게임 세션의 시스템을 연결하면서 PIE의 각 GameInstance, 서버 프로세스, 클라이언트 프로세스를 자연스럽게 분리한다. World별 복제가 목적이 아니므로 `UWorldSubsystem`보다 적합하다. 각 GameInstance는 독립 구독 Map을 가진다.

## 저장 구조와 Dispatch

`TMap<FGameplayTag, TArray<FSubscription>>`에 Tag별 Listener를 저장한다. 구독은 GUID, 약한 UObject 참조, Match Type, Native Delegate를 가진다. 발행 시 일치하는 GUID 스냅샷을 만든 뒤 실제 호출 직전에 구독 존재와 Listener 유효성을 다시 확인한다. 따라서 콜백 중 추가된 구독은 현재 Dispatch에 참여하지 않고, 제거된 구독은 이후 호출되지 않는다. 실행 Delegate는 복사해 자기 해제로 인한 저장소 무효화를 피한다.

호출 순서는 계약하지 않는다. `TMap` 순서나 등록 순서에 게임 규칙을 의존하면 안 된다. 순서가 필요한 명령은 직접 API/Interface로 표현한다.

## Thread와 재진입

발행, 등록, 해제는 Game Thread 전용이며 위반 시 `ensure` 후 거부한다. Dispatch는 동기식이고 정상적인 중첩 발행을 허용한다. Project Settings의 최대 깊이를 넘으면 추가 발행을 차단한다.

## UObject 수명

Listener는 `TWeakObjectPtr`로 추적한다. 파괴된 Listener는 Dispatch 전후 정리되며 호출되지 않는다. 메시지의 UObject 필드는 `UPROPERTY`/`TObjectPtr`이고 Payload는 `UObject` 기반이라 Blueprint와 GC에 안전하다. 이벤트는 동기식이므로 비동기 큐 수명은 제공하지 않는다.

## 네트워크 정책

발행은 로컬 프로세스의 해당 GameInstance 안에서만 일어난다. RPC, Replication, 서버-클라이언트 전달을 자동 수행하지 않는다. 도메인 시스템이 권한 확인과 상태 복제를 완료한 위치에서 서버/클라이언트에 필요한 로컬 사건을 각각 발행한다.

## Command와 Event

- Command: “문을 열어라”, “아이템을 추가하라”처럼 성공 여부와 즉시 반환이 필요한 요청. 직접 함수 또는 Interface를 유지한다.
- Event: “문이 열렸다”, “아이템이 추가됐다”처럼 이미 발생한 사실. `JMGameplayEvent`로 알린다.

## 의존 방향

```text
JMDoor / InventorySystem / ReusableDialogueSystem / JMInteraction
                              ↓
                       JMGameplayEvent
                              ↓
                 Unreal Engine Runtime modules
```

핵심 버스는 도메인 Tag와 구체 Payload를 모른다.
