---
title: "문제 해결"
status: ReviewRequired
authority: Guide
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# 문제 해결

## Inspector가 열리지 않음

확인할 것:

- 대상 Actor에 `UJMInspectableComponent`가 있는가
- `Inspection Data`가 지정되어 있는가
- 상호작용 실행자가 로컬 `APlayerController`로 연결되는가
- `LogJMGameplay` 경고가 출력되는가

AI나 서버 전용 Actor는 Local Player가 없으므로 Inspector UI를 열 수 없습니다.

## 상호작용 대상이 잡히지 않음

확인할 것:

- Trace Channel과 대상 Actor의 Collision Response가 맞는가
- Trace Distance가 충분한가
- `Trace Radius`가 너무 작지 않은가. 작은 물체나 얇은 콜라이더는 `20` 정도로 테스트합니다.
- `RefreshCurrentInteractable`을 호출하고 있는가
- Debug Draw를 켜서 라인트레이스 위치가 맞는지 확인했는가
- 조사 대상 Blueprint의 Static Mesh 또는 Collision Component에서 `Collision Enabled`가 `Query Only` 또는 `Query and Physics`인가
- 플레이어의 `Trace Channel`이 `Visibility`라면 대상 Component의 `Trace Responses -> Visibility`가 `Block`인가

처음 확인할 때 추천값:

```text
Trace Distance: 500
Trace Radius: 20
Trace Channel: Visibility
Draw Debug: true
```

Character Blueprint에 Camera가 붙어 있으면 기본적으로 Controller/Camera 시점 기준으로 Trace가 나갑니다. 그래도 Debug Draw 선이 엉뚱한 방향으로 나가면, 해당 Character가 `JMInteractorInterface`를 구현하고 있는지 확인합니다. 구현되어 있다면 `GetInteractionView`에서 반환하는 위치와 방향이 우선 사용됩니다.

Blueprint 안에 `ChildActorComponent`나 별도 충돌 Actor가 있으면 실제로 맞은 Actor와 `JMInspectableComponent`가 붙은 Actor가 다를 수 있습니다. 최신 코드는 맞은 Actor의 Owner와 Attach Parent까지 찾아보지만, 계속 실패하면 Trace에 직접 맞는 Actor가 들어 있는 Blueprint에 `JMInspectableComponent`를 붙입니다.

## ESC로 닫히지 않음

확인할 것:

- Inspector Widget이 Keyboard Focus를 가지고 있는가
- 커스텀 WBP가 `UJMItemInspectionWidgetBase`를 부모로 하고 있는가
- 외부 UI가 ESC 입력을 먼저 소비하고 있지 않은가

## Close Button이 동작하지 않음

기본 C++ 위젯의 Close Button은 자동으로 연결됩니다.

커스텀 WBP에서는 버튼 클릭 시 `RequestClose`를 직접 호출해야 합니다.

## Inspector를 닫아도 마우스 커서가 남음

확인할 것:

- 최신 코드가 빌드되어 있는가
- 커스텀 WBP에서 닫기 버튼이 `RequestClose`를 호출하는가
- 외부 PlayerController Blueprint가 Inspector가 닫힌 직후 `Show Mouse Cursor`를 다시 켜고 있지 않은가

기본 Subsystem은 Inspector를 닫을 때 `InputModeGameOnly`로 되돌리고, 열기 전 `bShowMouseCursor` 값으로 커서를 복구합니다.

## 이름이나 설명이 표시되지 않음

확인할 것:

- `UJMItemInspectionData` 에셋의 `DisplayName`과 `Description`을 입력했는가
- 에셋을 저장했는가
- 조사 대상 Actor의 `UJMInspectableComponent`에 있는 `Inspection Data`가 올바른 에셋을 가리키는가
- 커스텀 WBP를 쓰는 경우 `SetInspectionData` 또는 부모 클래스의 바인딩 흐름을 막고 있지 않은가
- 커스텀 WBP의 TextBlock 이름이 [WIDGET_SETUP.md](WIDGET_SETUP.md)의 이름표와 일치하는가

## Preview Mesh가 보이지 않음

확인할 것:

- `UJMItemInspectionData` 에셋의 `PreviewMesh`가 지정되어 있는가
- 지정한 Static Mesh가 로드 가능한 에셋인가
- 에셋을 저장했는가
- Inspector Preview 영역에 오류 안내 문구가 표시되는가

Preview Mesh가 비어 있거나 로드 실패하면 Inspector는 계속 열리고 Preview 영역에 안내 문구를 표시합니다.

## Preview에 검은 배경이 보임

확인할 것:

- 플러그인 콘텐츠에 `/ItemInspector/ItemInspection/M_JMItemPreviewAlpha` 머티리얼이 있는가
- 커스텀 WBP가 `JMItemInspectionWidgetBase`를 부모로 하고 있는가
- `PreviewImage`에 직접 Texture Brush를 고정해두지 않았는가
- `PreviewPanel` 또는 바깥 Border에 불투명 검은 배경색을 넣지 않았는가

기본 위젯은 Render Target을 직접 표시하지 않고, 프리뷰 알파 변환 머티리얼을 통해 표시합니다.

## Multiple directional lights 경고가 보임

경고 문구:

```text
Multiple directional lights are competing to be the single one used for forward shading...
```

현재 Preview Actor는 Directional Light를 만들지 않습니다. Inspector를 열 때도 이 경고가 계속 보인다면 다음을 확인합니다.

- 레벨 안에 Directional Light가 2개 이상 있지 않은가
- 다른 Blueprint나 플러그인이 런타임에 Directional Light를 추가하고 있지 않은가
- 기존 에디터 세션이 이전 DLL을 물고 있지 않은가. 에디터를 완전히 종료 후 다시 실행합니다.

이 플러그인의 Preview 조명은 Spot Key Light, Point Fill Light, Point Rim Light를 사용합니다. 그래서 레벨 Directional Light와 Forward Shading Priority 경쟁을 만들지 않습니다.

## Preview 조명 품질이 마음에 들지 않음

`UJMItemInspectionData` 에셋을 열고 `ViewSettings -> Lighting Settings`를 조정합니다.

- 전체가 어두움: `Intensity Multiplier`를 `1.2 ~ 1.5`로 올립니다.
- 전체가 너무 밝음: `Intensity Multiplier`를 `0.7 ~ 0.9`로 낮춥니다.
- 입체감이 부족함: `Lighting Preset`을 `WarmDramatic` 또는 `CoolRim`으로 바꿉니다.
- 정보 확인이 우선임: `Lighting Preset`을 `Flat`으로 바꿉니다.
- 그림자가 필요함: `Cast Preview Shadows`를 켭니다. 성능 비용이 늘 수 있으니 꼭 필요한 아이템에만 권장합니다.

## Preview 오브젝트가 완전히 검게 보임

최신 빌드인지 먼저 확인합니다. Preview 조명은 메시, 라이트, SceneCapture가 같은 Lighting/View Channel을 봐야 정상 작동합니다.

계속 검게 보이면 임시 확인용으로 `UJMItemInspectionData -> ViewSettings -> Lighting Settings`에서 다음처럼 설정합니다.

```text
Lighting Preset: BrightProduct
Intensity Multiplier: 1.5
Isolate From World Lighting: false
```

이 상태에서 밝아지면 프로젝트의 Lighting Channel 설정이나 다른 렌더 설정과 충돌하는 상황입니다. 기본 구현은 Preview Light의 Lighting Channel과 View Lighting Channel을 모두 맞춰서 생성합니다.

## 아이템이 너무 작거나 크게 보임

`UJMItemInspectionData` 에셋을 열고 `ViewSettings -> InitialZoom`을 조정합니다.

- 작게 보이면 `InitialZoom`을 `1.3 ~ 1.8`로 올립니다.
- 크게 보이면 `InitialZoom`을 `0.7 ~ 1.0`으로 낮춥니다.
- 휠 줌 범위가 부족하면 `MinZoom`, `MaxZoom`을 같이 조정합니다.

## Mesh가 회전하거나 줌되지 않음

확인할 것:

- 마우스가 Preview 영역 위에 있는가
- 왼쪽 버튼을 누른 채 드래그하고 있는가
- `ViewSettings`에서 `bAllowPitchRotation`, `bAllowYawRotation`, `bAllowZoom`이 꺼져 있지 않은가
- `RotationSensitivity`, `MinZoom`, `MaxZoom` 값이 너무 작거나 좁게 설정되어 있지 않은가

## 빌드 오류가 나는 경우

플러그인 Runtime 모듈 의존성은 다음 모듈을 포함합니다.

```text
Core
CoreUObject
Engine
DeveloperSettings
GameplayTags
InputCore
SlateCore
UMG
Slate
```

프로젝트 모듈에서 별도 의존성을 추가하지 않아도 기본 사용은 가능합니다.
