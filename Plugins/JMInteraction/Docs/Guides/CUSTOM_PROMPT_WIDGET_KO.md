---
title: "Custom Interaction Prompt Widget 만들기"
status: ReviewRequired
authority: Guide
scope: "Plugin: JMInteraction"
classified_on: 2026-08-19
owners:
  - "JMInteraction Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# Custom Interaction Prompt Widget 만들기

이 문서는 배경 박스 없이 Prompt 문구만 표시하는 `WBP_InteractionPrompt_TextOnly` 제작 절차를 설명한다.

## 1. Widget Blueprint 생성

1. Content Browser에서 프로젝트의 `/Game/UI` 폴더로 이동한다. 폴더가 없다면 만든다.
2. 우클릭 → **User Interface → Widget Blueprint**를 선택한다.
3. Parent Class 선택 창에서 **All Classes**를 열고 `JMInteractionPromptWidgetBase`를 검색한다.
4. 부모 클래스로 `JMInteractionPromptWidgetBase`를 선택한다.
5. 이름을 `WBP_InteractionPrompt_TextOnly`로 지정한다.

일반 `UserWidget`을 부모로 만들면 `JMInteractionComponent`의 Widget Class에 지정할 수 없다. 반드시 `JMInteractionPromptWidgetBase`의 자식이어야 한다.

## 2. 배경 없이 TextBlock만 배치

Designer에서 다음 구조를 만든다.

```text
Canvas Panel (Root)
└─ TextBlock: PromptText
```

`PromptText`는 다음과 같이 설정한다.

1. TextBlock 이름을 정확히 `PromptText`로 바꾼다.
2. Details에서 **Is Variable**을 켠다.
3. Canvas Panel Slot의 Anchor를 **Center**로 설정한다.
4. Alignment를 `X=0.5, Y=0.5`로 설정한다.
5. Position은 `X=0, Y=0`으로 둔다.
6. Auto Size를 켜는 것을 권장한다.
7. Font, 색상, Shadow Offset 등 게임 디자인 값을 설정한다.

위치 `Y=80`을 Designer에서 다시 입력하지 않는다. 중앙 Anchor 기준 Offset은 `JMInteraction`의 Prompt Style이 자동으로 적용한다. Designer와 Project Settings 양쪽에 Offset을 넣으면 두 번 이동한다.

`PromptText` 이름과 `Is Variable` 설정이 맞으면 베이스 클래스가 다음을 자동 처리한다.

- Prompt Text 변경
- Font family/typeface를 유지한 Font Size 변경
- Text Justification
- Screen Offset
- Render Scale
- 표시 및 숨김

Event Graph를 작성하지 않아도 기본 Prompt가 동작한다.

## 3. 배경 박스를 사용하지 않는 방법

문구만 필요하면 Designer에 Border를 추가하지 않는다. `Canvas Panel → PromptText` 구조만 사용한다.

이미 Border가 있다면 다음 중 하나를 선택한다.

- Border를 삭제하고 `PromptText`를 Canvas Panel의 직접 자식으로 옮긴다.
- Border의 Brush Color Alpha를 `0`으로 설정한다.
- Border Brush를 `None`으로 설정한다.

기본 C++ fallback Widget의 Border는 커스텀 Widget Class를 지정하지 않았을 때만 사용된다. `WBP_InteractionPrompt_TextOnly`를 설정하면 Designer 구조가 우선한다.

## 4. 선택적 배경/패딩 구조

나중에 배경이 필요하면 다음 구조를 사용할 수 있다.

```text
Canvas Panel (Root)
└─ Border: PromptContainer
   └─ TextBlock: PromptText
```

두 Widget 모두 **Is Variable**을 켜고 이름을 정확히 지정한다. `PromptContainer`가 있으면 Screen Offset, Render Scale, Padding은 Container 전체에 적용된다. 없으면 `PromptText`에 직접 적용된다.

## 5. Project Settings에 Widget 지정

1. **Edit → Project Settings**를 연다.
2. **Plugins → JM Interaction → Prompt UI**로 이동한다.
3. `Default Prompt Widget Class`에 `WBP_InteractionPrompt_TextOnly`를 지정한다.
4. 기본 스타일을 설정한다.

```text
Default Prompt Style
  Screen Offset: X=0, Y=80
  Font Size: 24
  Render Scale: X=1, Y=1
  Text Justification: Center
  Hidden Mode: Collapsed
```

양의 Y는 화면 아래쪽이다. `Y=80`이면 중앙 Dot 아래에 표시된다.

## 6. 특정 플레이어만 다르게 설정

Player Blueprint에서 `JMInteractionComponent`를 선택한다.

- `Interaction Prompt Widget Class`: 이 플레이어만 다른 Widget 사용
- `Override Prompt Style`: 전역 Prompt Style 대신 Component의 값 사용
- `Prompt Style`: Offset, Font Size, Scale, Padding 등 개별 설정

우선순위:

```text
Component Widget/Style Override
→ Project Settings Default Widget/Style
→ JMInteraction 기본 Native Widget/Style
```

## 7. BP_NPC Prompt 문구 변경

`BP_NPC`의 문구는 Widget이 아니라 NPC의 Interactable Component가 소유한다.

1. Content Browser의 **Settings → Show Plugin Content**를 켠다.
2. `/ReusableDialogueSystem/Examples/DialogueActor/BP_NPC`를 연다.
3. Components에서 `ReuseDialogueInteractableComponent`를 선택한다.
4. Details → **Dialogue → Interaction → Interaction Prompt**를 수정한다.

예:

```text
E키를 눌러 대화
```

Widget Class는 모든 Prompt의 모양을 정하고, `BP_NPC.InteractionPrompt`는 해당 NPC에 표시할 문구를 정한다.

## 8. Blueprint에서 API를 직접 재정의하는 경우

특수 애니메이션이 필요할 때만 Event Graph에서 다음 함수를 Override한다.

- `Set Prompt Text`
- `Set Prompt Visible`
- `Apply Prompt Style`
- `Set Interaction Prompt`
- `Clear Interaction Prompt`

기본 자동 바인딩을 유지하려면 Override 함수에서 **Add Call to Parent Function**을 호출한 뒤 Fade/Animation 로직을 추가한다. Parent 호출을 생략하면 `PromptText` 자동 갱신과 Project Settings 스타일 적용을 직접 구현해야 한다.

사용 가능한 이벤트:

- `On Prompt Shown`
- `On Prompt Hidden`
- `On Prompt Text Changed`
- `On Prompt Style Applied`

## 9. 문제 해결

Prompt가 보이지 않으면 다음을 확인한다.

- Designer에서 선택 테두리만 보이고 글자가 보이지 않으면 플러그인을 최신 빌드한 뒤 Editor를 완전히 재시작한다. 베이스 Widget은 Designer 미리보기에서 강제로 `Visible` 처리된다.
- Widget Blueprint 부모가 `JMInteractionPromptWidgetBase`인가
- TextBlock 이름이 정확히 `PromptText`이고 Is Variable이 켜져 있는가
- `Default Prompt Widget Class`가 지정되었는가
- Player의 `JMInteractionComponent.bCreatePromptWidget`이 켜져 있는가
- `Trace Mode`가 기본 `Timer`이거나 외부에서 Refresh를 지속 호출하는가
- 대상 Collision이 설정된 Trace Channel을 Block하는가
- 대상 Prompt의 `DisplayText`가 비어 있지 않고 `bVisible=true`인가

박스가 계속 보이면 실제 적용 중인 Widget Class를 확인한다. 커스텀 Class가 비어 있으면 플러그인의 Native fallback Widget이 사용된다.
