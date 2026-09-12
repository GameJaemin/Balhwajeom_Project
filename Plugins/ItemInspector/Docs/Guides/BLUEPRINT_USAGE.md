---
title: "Blueprint 사용법"
status: ReviewRequired
authority: Guide
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# Blueprint 사용법

## 상호작용 실행 Actor 설정

1. 상호작용을 실행할 Actor에 `UJMInteractionComponent`를 추가합니다.
2. `Trace Distance`, `Trace Radius`, `Trace Channel`, `Trace Mode`를 설정합니다.
3. 입력 이벤트에서 `Refresh Current Interactable`을 호출합니다.
4. 이어서 `Try Begin Interaction`을 호출합니다.

`Trace Mode`를 `Timer` 또는 `Tick`으로 설정했다면 입력 이벤트에서 Refresh를 생략할 수 있습니다.

처음 테스트할 때 추천값:

```text
Trace Distance: 500
Trace Radius: 20
Trace Channel: Visibility
Trace Mode: OnInput
Draw Debug: true
```

`Trace Radius`가 `0`이면 얇은 Line Trace입니다. Blueprint에서 대상이 잘 안 잡히면 `20`부터 테스트합니다.

## 조사 가능한 Actor 설정

1. 조사 대상 Actor에 `UJMInspectableComponent`를 추가합니다.
2. `Inspection Data`에 `UJMItemInspectionData` 에셋을 지정합니다.
3. `Prompt Text`를 원하는 문구로 변경합니다.

예시 프롬프트:

```text
조사하기
살펴보기
확인하기
```

## Blueprint Actor가 잘 안 잡힐 때 Collision 확인

상호작용은 `UJMInspectableComponent`를 직접 찾는 방식이 아니라, 플레이어 시점에서 Trace를 쏜 뒤 맞은 Actor에서 조사 가능 컴포넌트를 찾는 방식입니다. 그래서 Blueprint에 컴포넌트가 붙어 있어도 Static Mesh나 Collision Component가 Trace를 막지 않으면 상호작용 대상이 잡히지 않습니다.

가장 먼저 다음 순서로 확인합니다.

1. 조사 대상 Blueprint를 엽니다.
2. 왼쪽 `Components` 패널에서 실제로 눈에 보이는 `StaticMeshComponent` 또는 충돌용 `Box Collision`, `Sphere Collision`, `Capsule Collision`을 선택합니다.
3. 오른쪽 `Details` 패널에서 `Collision` 카테고리를 엽니다.
4. `Collision Enabled`를 `Query Only` 또는 `Query and Physics`로 둡니다.
5. `Collision Presets`가 `Custom`이라면 `Trace Responses`에서 `Visibility`를 `Block`으로 설정합니다.
6. 플레이어의 `JMInteractionComponent`에서 `Trace Channel`도 `Visibility`인지 확인합니다.
7. `Draw Debug`를 켜고 플레이해서 선이나 구가 대상 콜라이더에 닿는지 확인합니다.

`Trace Radius`가 0보다 크면 선 하나가 아니라 작은 구를 앞으로 굴리는 방식으로 검사합니다. 작은 열쇠, 얇은 문 손잡이, 콜라이더가 조금 작은 오브젝트는 `10 ~ 30` 정도가 훨씬 안정적입니다.

Blueprint 안에 `ChildActorComponent`를 쓰거나 별도의 충돌 Actor를 붙여 둔 구조도 지원합니다. 이 경우 실제로 맞은 Actor에 `JMInspectableComponent`가 없어도, 맞은 Actor의 Owner 또는 Attach Parent에 있는 `JMInspectableComponent`까지 찾아봅니다. 그래도 안 잡히면 가장 확실한 방법은 Trace에 맞는 바로 그 Actor나 Component가 들어 있는 Blueprint에 `JMInspectableComponent`를 붙이는 것입니다.

## Inspector Widget 교체

1. `UJMItemInspectionWidgetBase`를 부모로 하는 Widget Blueprint를 만듭니다.
2. Project Settings로 이동합니다.
3. `Plugins -> Item Inspector`를 엽니다.
4. `Default Inspector Widget Class`에 만든 WBP를 지정합니다.

커스텀 WBP에는 `PreviewPanel`, `PreviewImage`, `ItemNameText`, `DescriptionText`, `CloseButton` 같은 이름의 위젯을 추가해야 자동 연결됩니다.

전체 위젯 이름표와 배치 예시는 [WIDGET_SETUP.md](WIDGET_SETUP.md)를 참고합니다.

위젯 안의 닫기 버튼에서는 `RequestClose`를 호출합니다.

기본 C++ 위젯을 그대로 사용하면 별도 연결 없이 다음 항목이 표시됩니다.

- 아이템 이름
- 분류
- 설명
- 추가 정보
- Item ID
- Static Mesh Preview
- Preview 영역 마우스 드래그 회전
- Preview 영역 마우스 휠 줌
- Preview Mesh 누락 안내 문구

기본 C++ 위젯은 ESC와 `닫기 (ESC)` 버튼으로 닫힙니다.

## Blueprint에서 직접 Inspector 열기

미래의 인벤토리 UI나 퀘스트 UI에서는 `UJMItemInspectionSubsystem`을 얻은 뒤 `OpenInspection` 또는 `OpenInspectionFromRequest`를 호출하면 됩니다.

월드 Actor가 없어도 `InspectionData`만 있으면 열 수 있게 설계되어 있습니다.
