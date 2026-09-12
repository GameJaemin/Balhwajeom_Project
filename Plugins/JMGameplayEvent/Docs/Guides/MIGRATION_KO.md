---
title: "JMGameplayEvent Migration"
status: ReviewRequired
authority: Guide
scope: "Plugin: JMGameplayEvent"
classified_on: 2026-08-19
owners:
  - "JMGameplayEvent Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# JMGameplayEvent Migration

## 직접 호출 찾기

다른 플러그인의 헤더 include, `Build.cs`/`.uplugin` 의존성, `GetSubsystem`, 구체 클래스 Cast, Delegate binding을 함께 검색한다. 호출마다 요청(Command)인지 완료 사실(Event)인지 분류한다.

## 이벤트로 바꿀 호출

- 반환값을 쓰지 않고 이미 완료된 상태를 Objective/Save/Notification/Audio 등에 통지하는 호출
- 한 Publisher의 완료 사실을 여러 선택 소비자가 관찰하는 호출
- 소비자 플러그인이 없어도 Publisher 기능이 완전해야 하는 호출

## 직접 호출을 유지할 호출

- 즉시 성공/실패 결과가 필요한 요청
- Inventory `AddItem`, Door `OpenDoor`, Dialogue `StartDialogue` 같은 명령
- 명확한 소유 관계 내부의 함수
- 실제 동작을 수행해야 하는 Interface 호출

## 적용 내역

### JMDoor

기존 `OnDoorStateChanged`는 호환을 위해 유지했다. 실제 최종 상태 전이 후 `Event.Door.Opened`, `Closed`, `Locked`, `Unlocked`, `Broken`을 발행한다. Opening/Closing/Blocked 같은 중간 상태는 전역 사건으로 발행하지 않는다.

### InventorySystem

실제 슬롯 변경과 기존 Delegate 통지 후 `Event.Item.Acquired`, `Used`, `Removed`, `Dropped`을 발행한다. Payload에는 Item Definition, 작업 수량, 작업 후 전체 수량, Inventory Owner, Slot Index가 있다. Add 실패는 Acquired를 발행하지 않는다.

### ReusableDialogueSystem

Widget 생성과 모드 적용까지 성공한 뒤 `Event.Dialogue.Started`, 정상 Cleanup과 종료 Delegate 이후 `Event.Dialogue.Finished`를 발행한다. 현재 시스템에는 Choice 데이터/선택 API가 없어 `ChoiceSelected` Tag만 소유하며 발행 지점은 만들지 않았다.

### JMInteraction

최종 `CompleteInteract` 결과만 Succeeded/Failed로 발행하고, Begin 실패도 Failed로 발행한다. Focus는 실제 대상 변경 시에만 Started/Ended를 발행하며 매 Trace 프레임에는 발행하지 않는다.

기존 Delegate와 Blueprint API는 삭제하거나 이름을 바꾸지 않았다. 새 이벤트를 수신하는 시스템을 먼저 배포한 뒤, 중복 통지가 없는 것을 확인하면서 외부 직접 알림 호출을 단계적으로 제거한다.
