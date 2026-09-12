---
title: "JMGameplayEvent"
status: ReviewRequired
authority: Guide
scope: "Plugin: JMGameplayEvent"
classified_on: 2026-08-19
owners:
  - "JMGameplayEvent Maintainers"
review_reason: "설치 closure와 사용 절차를 현재 .uplugin·Build.cs·코드와 완전히 재검증하지 않음"
---

# JMGameplayEvent

## 문서 지도

> 아래 상태는 문서 내용의 검증 수준이다. `ReviewRequired` 문서는 현재 코드·에셋·Editor 절차와 다시 확인하기 전까지 최종 구현 기준으로 사용하지 않는다.

| 분류 | 문서 | 상태 |
|---|---|---|
| Docs Root | [JMGameplayEvent 아키텍처](ARCHITECTURE_KO.md) | `ReviewRequired` |
| Docs Root | [Changelog](CHANGELOG.md) | `ReviewRequired` |
| Guides | [JMGameplayEvent Migration](Guides/MIGRATION_KO.md) | `ReviewRequired` |


`JMGameplayEvent`는 UE 5.7용 Runtime Gameplay Tag 이벤트 버스다. 기능을 실행하는 명령 버스가 아니라, 이미 발생한 사건을 서로 모르는 시스템에 동기적으로 알린다. 핵심 모듈은 Door, Inventory, Dialogue, Interaction 등 어떤 도메인 플러그인에도 의존하지 않는다.

## 설치와 의존성

`Plugins/JMGameplayEvent` 폴더를 복사하고 플러그인을 활성화한다. Runtime 의존성은 `Core`, `CoreUObject`, `Engine`, `GameplayTags`, `DeveloperSettings`뿐이다. 기존 기능 플러그인이 이벤트를 발행하려면 해당 기능 쪽에서 `JMGameplayEvent`를 의존해야 한다.

## C++ 발행

```cpp
UJMGameplayEventSubsystem* Bus = GetGameInstance()->GetSubsystem<UJMGameplayEventSubsystem>();
FJMGameplayEventMessage Message;
Message.EventTag = MyFeatureEventTags::Completed;
Message.Source = this;
Message.Instigator = InstigatorActor;
Message.Target = TargetObject;
Message.Payload = DomainPayload;
Bus->PublishEvent(Message);
```

기능 전용 Tag와 Payload는 발행 기능 플러그인이 소유한다. `JMGameplayEvent` 공통 메시지에 도메인 필드를 추가하지 않는다.

## C++ 구독

```cpp
FJMGameplayEventNativeDelegate Callback;
Callback.BindUObject(this, &UMyListener::HandleGameplayEvent);
Handle = Bus->SubscribeEvent(
    MyFeatureEventTags::Root,
    this,
    EJMGameplayEventMatchType::IncludeChildren,
    MoveTemp(Callback));

Bus->UnsubscribeEvent(Handle);
// 또는 Listener의 모든 핸들 제거
Bus->UnsubscribeAll(this);
```

`Exact`는 동일 Tag만, `IncludeChildren`은 구독 Tag 자신과 자식 Tag를 받는다. 동일 Listener/Tag의 중복 구독은 허용하며 각 핸들마다 한 번 호출된다. Project Settings의 경고로 실수를 탐지할 수 있다.

## Blueprint 발행과 수신

- `Publish Gameplay Event` 노드에 World Context, Event Tag, Source, Instigator, Target, Context Tags, Payload를 전달한다.
- Actor에 `JMGameplayEventListenerComponent`를 추가한다.
- `Event Tags`, `Match Type`, `Auto Subscribe`를 설정하고 `On Gameplay Event Received`에 바인딩한다.
- Component는 BeginPlay에서 등록하고 EndPlay에서 자동 해제한다.
- 런타임 변경은 `Add Event Tag`, `Remove Event Tag`, `Refresh Subscriptions`를 사용한다.

## Payload 확장

기능 플러그인에서 `UJMGameplayEventPayloadBase`를 상속한 `BlueprintType` 클래스를 만든다. Payload는 동기 Dispatch 중 유효하고, 장기 보관하려면 Listener가 명시적으로 강한 참조를 보유해야 한다.

## 설정

`Project Settings > Plugins > JM Gameplay Event`에서 발행/미처리 이벤트 로그, 중복 구독 경고, 최대 중첩 Dispatch 깊이를 설정한다. Shipping에서는 디버그 로그가 강제로 비활성화된다.
