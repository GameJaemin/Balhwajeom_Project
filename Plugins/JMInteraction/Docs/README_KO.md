---
title: "JMInteraction 사용 가이드"
status: ReviewRequired
authority: Guide
scope: "Plugin: JMInteraction"
classified_on: 2026-08-19
owners:
  - "JMInteraction Maintainers"
review_reason: "설치 closure와 사용 절차를 현재 .uplugin·Build.cs·코드와 완전히 재검증하지 않음"
---

# JMInteraction 사용 가이드

## 문서 지도

> 아래 상태는 문서 내용의 검증 수준이다. `ReviewRequired` 문서는 현재 코드·에셋·Editor 절차와 다시 확인하기 전까지 최종 구현 기준으로 사용하지 않는다.

| 분류 | 문서 | 상태 |
|---|---|---|
| Archive | [기존 프로젝트 분석 및 Migration 기록](Archive/PROJECT_ANALYSIS.md) | `Archived` |
| Docs Root | [Architecture](ARCHITECTURE.md) | `ReviewRequired` |
| Docs Root | [Changelog](CHANGELOG.md) | `ReviewRequired` |
| Guides | [Custom Interaction Prompt Widget 만들기](Guides/CUSTOM_PROMPT_WIDGET_KO.md) | `ReviewRequired` |


## 1. 개요

`JMInteraction`은 대상 탐색, 거리 검사, Focus 전환, Prompt UI, 상호작용 실행만 담당하는 독립 Runtime 플러그인이다. Door, Inventory, Dialogue 같은 기능은 `IJMInteractableInterface`를 구현하고 실제 행동을 스스로 수행한다. 핵심 모듈은 해당 기능 플러그인을 참조하지 않는다.

## 2. 플레이어에 InteractionComponent 추가

플레이어 Character 또는 PlayerController Blueprint에 `JMInteractionComponent`를 추가한다. 프로젝트 기본값은 **Project Settings > Plugins > JM Interaction**에서 설정한다. 특정 플레이어만 값을 바꾸려면 컴포넌트의 `Override Project Settings`를 켜고 거리, 채널, 탐색 방식, 갱신 방식을 지정한다.

현재 프로젝트의 `BP_FirstPersonCharacter`에는 기존 `JMInteractionComponent`가 이미 있으며 Core Redirect를 통해 새 모듈 클래스로 이전된다.

## 3. 입력 연결

입력 시스템은 대상 타입을 알 필요가 없다.

```cpp
void AMyCharacter::Interact()
{
    InteractionComponent->TryInteract();
}
```

Blueprint에서는 E 또는 `IA_Interact`의 `Started`에서 `JMInteractionComponent -> Try Interact`를 호출한다. 플러그인은 Input Mapping Context를 추가하거나 변경하지 않는다. 현재 `BP_FirstPersonCharacter`는 E 입력에서 기존 `Try Begin Interaction`을 호출하며 이 API도 호환 유지된다.

## 4. 새 Interactable Actor 만들기

Blueprint 전용 제작 흐름:

1. Actor Blueprint를 만든다.
2. `JMInteractableComponent`를 추가한다.
3. `Interaction Prompt`를 지정한다.
4. `On Interacted` 이벤트에 실제 행동을 연결한다.

복잡한 조건이나 Hold 동작이 필요하면 Actor/Component에서 `JMInteractableInterface`를 구현하고 `CanInteract`, `BeginInteract`, `CompleteInteract`, `GetInteractionPrompt`를 재정의한다. `OnFocusBegin`과 `OnFocusEnd`는 대상이 실제로 바뀔 때 한 번씩만 호출된다.

## 5. Custom Prompt

Prompt는 `FText`이며 키 이름을 핵심 코드가 하드코딩하지 않는다. 게임에서 완성 문구를 지정한다.

```text
BP_Item: E키를 눌러 획득
BP_Door: E키를 눌러 열기
BP_NPC:  E키를 눌러 대화
```

입력 리바인딩 키를 자동 반영하려면 게임 측에서 현재 키를 조회해 `GetInteractionPrompt`의 `DisplayText`를 구성한다.

## 6. CenterScreenTrace

`Detection Mode = Center Screen Trace`로 설정한다. `JMInteractorInterface.GetInteractionView`가 구현되어 있으면 그 위치/방향을 우선 사용하고, 없으면 Controller 시점, CameraComponent, Actor 전방 순으로 사용한다. `Trace Distance`, `Trace Radius`, `Trace Channel`을 설정할 수 있다.

## 7. MouseCursorTrace

`Detection Mode = Mouse Cursor Trace`로 설정한다. 로컬 PlayerController의 커서 아래 Hit를 사용하고, Hit 지점이 `Trace Distance` 밖이면 거부한다. 커서 표시와 Input Mode 설정은 호스트 게임 책임이다.

## 8. Custom Interaction Widget

기본 `JMInteractionPromptWidgetBase`는 최소 텍스트 UI를 제공한다. 게임 전용 Widget Blueprint를 이 클래스로부터 만들고 `Set Interaction Prompt`/`Clear Interaction Prompt`를 재정의한 뒤 Project Settings의 `Default Prompt Widget Class` 또는 컴포넌트의 `Interaction Prompt Widget Class`에 지정한다. Dedicated Server에서는 Widget을 만들지 않는다.

배경 없는 Text 전용 Widget을 만드는 전체 Designer 절차는 [CUSTOM_PROMPT_WIDGET_KO.md](Guides/CUSTOM_PROMPT_WIDGET_KO.md)를 참고한다. `PromptText` TextBlock과 선택적 `PromptContainer` Border를 `BindWidgetOptional`로 지원하므로 기본적인 경우 Event Graph 구현이 필요 없다.

인스펙터나 인벤토리 같은 Modal UI는 `SetPromptSuppressed(true)`로 Prompt만 일시 중지하고, 닫힐 때 기존 상태로 복원할 수 있다. 탐지와 상호작용 자체는 비활성화되지 않는다. `ItemInspector` 플러그인은 검사 화면이 열리고 닫힐 때 이 처리를 자동으로 수행한다.

### Prompt 표시 시점과 수명

Prompt Widget은 `InteractionComponent.BeginPlay`에서 한 번만 생성되고 처음에는 `Collapsed` 또는 `Hidden`이다. Timer/Tick Trace가 대상을 감지하면 입력과 무관하게 다음 순서로 즉시 표시된다.

```text
Detect → Focus → GetInteractionPrompt → Show Prompt → Input → Interact → Refresh/Hide
```

같은 대상을 계속 바라볼 때는 새 Prompt가 이전 값과 다를 때만 Text와 Visibility를 갱신한다. 상호작용 완료 후에는 즉시 재탐색하므로 획득되어 파괴된 Item은 Prompt가 숨겨지고, 열린 Door는 `E키를 눌러 닫기`로 바뀐다.

### 전역 Prompt UI 설정

**Project Settings > Plugins > JM Interaction > Prompt UI**에서 설정한다.

```text
Default Prompt Style
  Screen Offset: X=0, Y=80
  Font Size: 24
  Render Scale: X=1, Y=1
  Padding: Left/Top/Right/Bottom
  Text Justification: Center
  Hidden Mode: Collapsed 또는 Hidden
```

양의 Screen Offset Y는 화면 중앙에서 아래쪽이다. 기본 `Y=80`은 중앙 Dot 아래에 Prompt를 배치한다. 해상도별 절대 좌표가 아니라 전체 화면 중앙 기준 내부 컨테이너 이동이므로 1280×720, 1920×1080, 2560×1440에서도 기준점이 유지된다.

`Font Size`는 텍스트의 실제 레이아웃 크기이며 기존 Font family/typeface를 유지한 채 Size만 변경한다. `Render Scale`은 Padding과 배경을 포함한 완성된 Prompt 컨테이너 전체 배율이다.

### 플레이어별 Override

`JMInteractionComponent`에서 `Override Prompt Style`을 켜고 `Prompt Style` 값을 편집한다. 우선순위는 다음과 같다.

```text
Component Prompt Style → Project Settings Default Prompt Style → 플러그인 내부 기본값
```

탐색 설정의 `Override Project Settings`와 Prompt Style Override는 독립적이다.

### Custom Widget API

`UJMInteractionPromptWidgetBase` Blueprint subclass에서 다음 이벤트/API를 재정의할 수 있다.

- `Set Prompt Text`
- `Set Prompt Visible`
- `Apply Prompt Style`
- `On Prompt Shown`, `On Prompt Hidden`
- `On Prompt Text Changed`, `On Prompt Style Applied`

InteractionComponent는 Widget 내부의 TextBlock 이름을 검색하지 않는다.

## 9. 다른 플러그인에서 사용

소비 플러그인은 Public 헤더에서 JMInteraction 타입을 노출하면 Public dependency로, Private 구현에서만 사용하면 Private dependency로 추가한다. 의존 방향은 항상 다음과 같다.

```text
Inventory / Dialogue / Door Integration -> JMInteraction -> Unreal Engine
```

`JMInteraction` 내부에서 소비 플러그인의 구체 클래스로 Cast하지 않는다.

## 10. Plugin Dependency 설정

`.uplugin`:

```json
"Plugins": [
  { "Name": "JMInteraction", "Enabled": true }
]
```

`Build.cs`:

```csharp
PublicDependencyModuleNames.Add("JMInteraction");
```

플러그인 폴더만 다른 UE 5.7 프로젝트의 `Plugins/JMInteraction`에 복사하고 활성화할 수 있다. 빌드 산출물인 `Binaries`와 `Intermediate`는 배포에서 제외한다.

## 갱신과 디버그

`Trace Mode`는 `Manual`, `OnInput`, `Timer`, `Tick`을 지원한다. 기본은 입력 전에 Prompt를 감지하는 `Timer`이며 간격은 0.1초다. `Manual`은 외부 코드가 지속적으로 `RefreshCurrentInteractable`을 호출할 때만 사용한다. `Debug Trace`를 켜면 Development에서 Focus, Prompt 갱신/표시/숨김, 입력, 실행 로그가 출력되며 Shipping/Test 빌드에서는 제거된다.
