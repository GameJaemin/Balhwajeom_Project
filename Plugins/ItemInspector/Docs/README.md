---
title: "ItemInspector 초보자 사용 안내"
status: ReviewRequired
authority: Guide
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "설치 closure와 사용 절차를 현재 .uplugin·Build.cs·코드와 완전히 재검증하지 않음"
---

# ItemInspector 초보자 사용 안내

## 문서 지도

> 아래 상태는 문서 내용의 검증 수준이다. `ReviewRequired` 문서는 현재 코드·에셋·Editor 절차와 다시 확인하기 전까지 최종 구현 기준으로 사용하지 않는다.

| 분류 | 문서 | 상태 |
|---|---|---|
| Docs Root | [아키텍처](ARCHITECTURE.md) | `ReviewRequired` |
| Docs Root | [변경 기록](CHANGELOG.md) | `ReviewRequired` |
| Guides | [Blueprint 사용법](Guides/BLUEPRINT_USAGE.md) | `ReviewRequired` |
| Guides | [데이터 작성법](Guides/DATA_AUTHORING.md) | `ReviewRequired` |
| Guides | [다른 프로젝트에 이식하기](Guides/PORTABLE_INSTALL.md) | `ReviewRequired` |
| Guides | [문제 해결](Guides/TROUBLESHOOTING.md) | `ReviewRequired` |
| Guides | [Inspector Widget 제작 및 이식 가이드](Guides/WIDGET_SETUP.md) | `ReviewRequired` |
| Plans | [구현 계획](Plans/IMPLEMENTATION_PLAN.md) | `Draft` |
| Plans | [TODO](Plans/TODO.md) | `Draft` |
| Reference | [상호작용 시스템](Reference/INTERACTION_SYSTEM.md) | `ReviewRequired` |
| Reference | [아이템 조사 시스템](Reference/ITEM_INSPECTION_SYSTEM.md) | `ReviewRequired` |


`ItemInspector`는 Unreal Engine 프로젝트에서 반복해서 만들게 되는 상호작용과 아이템 조사 기능을 재사용하기 위한 Runtime Plugin입니다.

이 문서는 언리얼 에셋, 컴포넌트, 데이터 에셋을 처음 쓰는 사람도 따라갈 수 있게 작성했습니다.

## 지금 구현된 기능

현재 가능한 일:

- 플레이어 같은 Actor에 `UJMInteractionComponent`를 붙여 라인트레이스로 상호작용 대상을 찾기
- 월드에 배치된 Actor에 `UJMInspectableComponent`를 붙여 "조사 가능한 대상"으로 만들기
- `UJMItemInspectionData` 데이터 에셋에 아이템 이름과 설명을 작성하기
- 조사 가능한 Actor와 상호작용하면 기본 Inspector 화면 열기
- Inspector 화면에 이름, 분류, 설명, 추가 정보, Item ID 표시하기
- `PreviewMesh`를 별도 Runtime Preview Actor에 표시하기
- SceneCapture2D가 찍은 Render Target을 Inspector UI에 표시하기
- Preview 영역에서 마우스 왼쪽 드래그로 Mesh 회전하기
- Preview 영역에서 마우스 휠로 확대/축소하기
- Inspector가 열려 있는 동안 플레이어 이동/시점 입력 잠그기
- Inspector Preview에서 하늘 배경을 끄고 투명 배경 위에 아이템만 표시하기
- Inspector Preview 전용 조명 프리셋과 커스텀 조명 강도/색상 조정하기
- ESC 또는 `닫기 (ESC)` 버튼으로 Inspector 닫기

아직 남아 있는 일:

- Async Load로 큰 Mesh를 부드럽게 로드하기
- Demo Map과 자동 테스트 추가하기

즉, 지금 단계에서는 "조사 화면을 열고, 이름/설명을 표시하고, Static Mesh를 돌려보는 구조"까지 사용할 수 있습니다.

## 이번 버전에서 자주 확인할 설정

Inspector가 열렸는데 플레이어가 계속 움직이면 다음 두 위치를 확인합니다.

1. 조사 대상 Actor를 선택합니다.
2. `Details` 패널에서 `JMInspectableComponent`를 선택합니다.
3. `JM Gameplay | Item Inspection` 카테고리를 엽니다.
4. `Block Player Input During Inspection`이 켜져 있는지 확인합니다.

기본값은 켜짐입니다. 이 옵션이 켜져 있으면 Inspector가 열려 있는 동안 이동 입력과 마우스 시점 입력이 잠기고, Inspector를 닫으면 다시 풀립니다.

Inspector를 닫을 때는 마우스 커서와 입력 모드도 원래 게임 조작 상태로 되돌립니다. 일반적인 1인칭/3인칭 플레이처럼 원래 커서가 꺼져 있던 상태라면, 닫기 버튼을 눌러도 커서가 다시 사라집니다.

게임 시간까지 멈추고 싶다면 같은 위치에서 `Pause Game During Inspection`도 켭니다. 이 옵션은 월드의 시간이 멈추는 방식이므로, UI 애니메이션이나 별도 시스템이 일시정지 중에도 동작해야 하는 프로젝트에서는 먼저 테스트해보는 것을 권장합니다.

프로젝트 전체 기본값은 다음 위치에서 바꿀 수 있습니다.

```text
Edit
-> Project Settings
-> Plugins
-> Item Inspector
-> Item Inspection
-> Default Block Player Input
-> Default Pause Game
```

Preview 화면에 하늘이 보이거나 아이템이 너무 작게 보이면 `UJMItemInspectionData` 에셋을 열고 `ViewSettings`를 확인합니다.

- `InitialZoom`: Inspector를 열었을 때 처음 보이는 확대값입니다. 작은 물건은 `1.3`, `1.5`, `1.8`처럼 올립니다.
- `PreviewScale`: Mesh 자체의 스케일 보정값입니다. 보통은 `1.0`으로 두고, 원본 Mesh 크기나 Pivot이 특이할 때만 조정합니다.
- `RotationSensitivity`: 마우스 드래그 회전 속도입니다. 기본값은 `0.35`이고, 느리면 `0.5` 정도로 올립니다.
- `MinZoom`, `MaxZoom`: 마우스 휠 확대/축소 범위입니다.
- `LightingSettings`: 프리뷰 전용 조명 스타일입니다. `SoftStudio`, `BrightProduct`, `WarmDramatic`, `CoolRim`, `Flat` 중에서 고르거나 `Override Preset`을 켜서 Key/Fill/Rim 조명을 직접 조정합니다.

기본 Preview Render Target은 알파용 UI 머티리얼을 거쳐 표시되므로 하늘이나 검은 배경 없이 아이템만 보이도록 구성되어 있습니다. 커스텀 Widget Blueprint를 쓰는 경우에는 `PreviewPanel` 또는 `PreviewImage` 뒤에 불투명한 배경색을 칠하면 투명 배경처럼 보이지 않을 수 있습니다. 이 부분은 [WIDGET_SETUP.md](Guides/WIDGET_SETUP.md)의 PreviewPanel 권장값을 같이 확인하세요.

Preview 조명은 레벨의 Directional Light와 경쟁하지 않도록 Directional Light를 사용하지 않습니다. 대신 Preview Actor 내부의 Spot Key Light, Point Fill Light, Point Rim Light를 사용하고, 기본값으로 Lighting Channel 2에 격리합니다.

## 먼저 알아둘 용어

### Actor

레벨에 배치되는 물체입니다.

예:

- 큐브
- 문
- 아이템
- 캐릭터
- 트리거 박스

### Component

Actor에 붙이는 기능 조각입니다.

예를 들어 어떤 큐브 Actor에 `UJMInspectableComponent`를 붙이면 그 큐브는 "조사 가능한 Actor"가 됩니다.

### Data Asset

콘텐츠 브라우저에 저장되는 데이터 파일입니다.

코드를 직접 수정하지 않고 아이템 이름, 설명, Preview Mesh 같은 값을 입력하기 위해 사용합니다.

이 플러그인에서는 `UJMItemInspectionData`가 아이템 조사용 Data Asset입니다.

### Inspection Data

`Inspection Data`는 파일 이름이 아니라 `UJMInspectableComponent` 안에 있는 속성 이름입니다.

언리얼 에디터에서 조사 대상 Actor를 선택하고, 그 Actor에 붙어 있는 `UJMInspectableComponent`를 선택하면 Details 패널에 `Inspection Data`라는 칸이 보입니다.

그 칸에 `UJMItemInspectionData` 에셋을 지정합니다.

## 전체 사용 흐름

가장 기본 흐름은 다음과 같습니다.

```text
1. UJMItemInspectionData 에셋을 만든다.
2. 레벨에 조사할 Actor를 배치한다.
3. 그 Actor에 UJMInspectableComponent를 붙인다.
4. UJMInspectableComponent의 Inspection Data 칸에 1번에서 만든 에셋을 넣는다.
5. 플레이어 Actor에 UJMInteractionComponent를 붙인다.
6. 입력 이벤트에서 Try Begin Interaction을 호출한다.
7. 조사 대상과 상호작용하면 Inspector 화면이 열린다.
```

## 1단계: Item Inspection Data 에셋 만들기

아이템 이름과 설명을 담을 데이터 에셋을 먼저 만듭니다.

1. Unreal Editor에서 `Content Browser`를 엽니다.
2. `Content` 폴더 안에 원하는 폴더를 만듭니다.
   예: `Content/Data/Inspection`
3. 빈 공간에서 마우스 오른쪽 버튼을 누릅니다.
4. `Miscellaneous` 또는 `기타` 메뉴에서 `Data Asset`을 선택합니다.
5. 클래스 선택 창이 뜨면 검색창에 `JMItemInspectionData`를 입력합니다.
6. `JMItemInspectionData`를 선택하고 생성합니다.
7. 에셋 이름을 정합니다.
   예: `DA_OldKey_Inspection`

이제 콘텐츠 브라우저에 `DA_OldKey_Inspection` 같은 데이터 에셋이 생깁니다.

## 2단계: Item Inspection Data 값 입력하기

방금 만든 `DA_OldKey_Inspection` 에셋을 더블클릭합니다.

Details 패널에서 다음 값을 입력합니다.

- `ItemId`: 내부 식별자입니다. 예: `OldKey`
- `DisplayName`: Inspector 오른쪽에 크게 보일 이름입니다. 예: `낡은 열쇠`
- `DisplayCategory`: 이름 아래에 작게 보일 분류입니다. 예: `열쇠`, `단서`, `퀘스트 아이템`
- `Description`: Inspector 오른쪽 설명 영역에 보일 본문입니다. 예: `오래된 교실 문을 열 수 있을 것 같은 열쇠다.`
- `AdditionalInfo`: 설명 아래에 추가로 보일 짧은 정보입니다. 예: `손잡이 부분에 2-B라고 적혀 있다.`
- `PreviewMesh`: Inspector 왼쪽 Preview 영역에 표시할 Static Mesh입니다. 비워 두면 Inspector는 열리지만 Preview 영역에는 안내 문구가 표시됩니다.
- `MaterialOverrides`: Preview Mesh에 덮어쓸 Material입니다. 필요 없으면 비워도 됩니다.
- `ViewSettings`: 회전, 줌, 스케일에 사용할 설정입니다. 기본값으로 둬도 됩니다.

기본 Inspector 위젯은 `DisplayName`, `DisplayCategory`, `Description`, `AdditionalInfo`, `ItemId`, `PreviewMesh`를 사용합니다.

입력 후 에셋을 저장합니다.

## 3단계: 조사 가능한 Actor 만들기

이제 레벨에 있는 물체를 조사 가능한 대상으로 만듭니다.

### 방법 A: 레벨에 이미 배치된 Actor에 붙이기

1. 레벨에서 조사 대상으로 쓸 Actor를 선택합니다.
   예: 큐브, Static Mesh Actor, 문 Actor
2. 오른쪽 `Details` 패널을 봅니다.
3. `Add Component` 버튼을 누릅니다.
4. 검색창에 `JMInspectable`을 입력합니다.
5. `JMInspectableComponent`를 선택합니다.
6. Actor의 컴포넌트 목록에서 `JMInspectableComponent`를 클릭합니다.
7. Details 패널에서 `JM Gameplay | Item Inspection` 카테고리를 찾습니다.
8. 그 안의 `Inspection Data` 칸에 1단계에서 만든 `DA_OldKey_Inspection` 에셋을 지정합니다.

지정 방법은 둘 중 편한 방식을 쓰면 됩니다.

- `Inspection Data` 오른쪽 드롭다운을 눌러 에셋 선택
- 콘텐츠 브라우저의 `DA_OldKey_Inspection` 에셋을 `Inspection Data` 칸으로 드래그 앤 드롭

`Inspection Data`가 비어 있으면 이 Actor는 조사 화면을 열 수 없습니다.

### 방법 B: Blueprint Actor에 기본값으로 넣기

여러 개의 같은 아이템을 배치할 예정이라면 Blueprint Actor에 컴포넌트를 넣는 편이 좋습니다.

1. 콘텐츠 브라우저에서 Actor Blueprint를 만듭니다.
   예: `BP_OldKey`
2. `BP_OldKey`를 엽니다.
3. 왼쪽 `Components` 패널에서 `Add`를 누릅니다.
4. `JMInspectableComponent`를 추가합니다.
5. 추가한 `JMInspectableComponent`를 선택합니다.
6. Details 패널의 `Inspection Data`에 `DA_OldKey_Inspection`을 지정합니다.
7. Compile 후 Save 합니다.
8. 레벨에 `BP_OldKey`를 배치합니다.

이 방식은 같은 설정을 가진 조사 아이템을 여러 개 만들 때 편합니다.

## 4단계: 상호작용을 실행할 Actor 준비하기

조사 대상만 만들어서는 Inspector가 열리지 않습니다.

플레이어가 "무엇을 바라보고 있는지" 찾는 컴포넌트가 필요합니다. 그 역할을 `UJMInteractionComponent`가 합니다.

### 이미 플레이어 Character Blueprint가 있는 경우

1. 플레이어 Character Blueprint를 엽니다.
2. 왼쪽 `Components` 패널에서 `Add`를 누릅니다.
3. `JMInteraction`을 검색합니다.
4. `JMInteractionComponent`를 추가합니다.
5. Details 패널에서 기본값을 확인합니다.

추천 기본값:

- `Trace Distance`: 500
- `Trace Channel`: Visibility
- `Trace Mode`: Manual 또는 OnInput
- `Draw Debug`: 처음 테스트할 때만 켜기

### 플레이어 Blueprint가 아직 없는 경우

이 플러그인은 특정 Player Character를 자동으로 만들어 주지 않습니다.

테스트를 하려면 프로젝트에 플레이어 Pawn 또는 Character Blueprint가 필요합니다. 기존 게임 프로젝트라면 이미 조작 중인 Character에 `JMInteractionComponent`를 붙이면 됩니다.

빈 프로젝트라면 먼저 테스트용 Character Blueprint를 만든 뒤, GameMode 또는 World Settings에서 그 Character를 기본 Pawn으로 설정해야 합니다.

## 5단계: 입력 이벤트에서 상호작용 호출하기

이 플러그인은 키보드 입력을 직접 묶지 않습니다.

왜냐하면 프로젝트마다 입력 방식이 다르기 때문입니다.

- 어떤 프로젝트는 Enhanced Input을 사용합니다.
- 어떤 프로젝트는 기존 Input Action을 사용합니다.
- 어떤 프로젝트는 UI 버튼으로 상호작용할 수도 있습니다.

따라서 게임 쪽에서 입력을 받은 뒤 `UJMInteractionComponent` 함수를 호출해야 합니다.

### Blueprint 예시

플레이어 Character Blueprint에서 상호작용 입력 이벤트를 만든 뒤 다음 순서로 노드를 연결합니다.

```text
입력 이벤트
-> JMInteractionComponent 참조
-> Refresh Current Interactable
-> Try Begin Interaction
```

`Trace Mode`를 `OnInput`으로 설정했다면 `Try Begin Interaction` 내부에서 필요할 때 탐색을 시도합니다. 그래도 처음에는 `Refresh Current Interactable`을 직접 호출해 흐름을 눈으로 확인하는 편이 좋습니다.

### C++ 예시

```cpp
if (UJMInteractionComponent* InteractionComponent = FindComponentByClass<UJMInteractionComponent>())
{
    InteractionComponent->RefreshCurrentInteractable();
    const FJMInteractionResult Result = InteractionComponent->TryBeginInteraction();
}
```

## 6단계: 테스트하기

1. 레벨에 조사 대상 Actor가 있는지 확인합니다.
2. 조사 대상 Actor에 `UJMInspectableComponent`가 붙어 있는지 확인합니다.
3. `UJMInspectableComponent`의 `Inspection Data`가 비어 있지 않은지 확인합니다.
4. 플레이어 Actor에 `UJMInteractionComponent`가 붙어 있는지 확인합니다.
5. 플레이어가 조사 대상 Actor를 바라봅니다.
6. 상호작용 입력을 누릅니다.
7. Inspector 화면이 열리는지 확인합니다.
8. 오른쪽 패널에 이름, 분류, 설명, 추가 정보, ID가 표시되는지 확인합니다.
9. 왼쪽 Preview 영역에 Static Mesh가 보이는지 확인합니다.
10. Preview 영역에서 마우스 왼쪽 버튼을 누른 채 드래그해서 Mesh가 회전하는지 확인합니다.
11. Preview 영역에서 마우스 휠을 움직여 확대/축소가 되는지 확인합니다.
12. ESC 또는 `닫기 (ESC)` 버튼으로 닫히는지 확인합니다.

처음 테스트할 때는 `UJMInteractionComponent`의 `Draw Debug`를 켜면 라인트레이스가 어디로 나가는지 볼 수 있습니다.

## Blueprint에서 상호작용이 잘 안 될 때

Blueprint Actor에서 조사 기능이 안 잡히면 대부분은 코드보다 Collision 설정 문제입니다. `UJMInspectableComponent`가 붙어 있어도, 플레이어가 쏜 Trace가 실제 Mesh나 Collision Component에 막히지 않으면 대상이 없는 것으로 처리됩니다.

먼저 플레이어 Character Blueprint의 `JMInteractionComponent`를 선택하고 다음처럼 설정합니다.

```text
Trace Distance: 500
Trace Radius: 20
Trace Channel: Visibility
Trace Mode: OnInput
Draw Debug: true
```

그 다음 조사 대상 Blueprint를 열고, `Components` 패널에서 실제로 맞아야 하는 `StaticMeshComponent`나 `Box Collision`을 선택합니다. 오른쪽 `Details` 패널의 `Collision` 카테고리에서 다음을 확인합니다.

- `Collision Enabled`: `Query Only` 또는 `Query and Physics`
- `Trace Responses -> Visibility`: `Block`
- 플레이어 쪽 `Trace Channel`: 대상이 Block하는 채널과 같아야 함

`Draw Debug`를 켜면 플레이 중에 초록색 또는 빨간색 선이 보입니다. `Trace Radius`가 0보다 크면 선 양 끝과 충돌 지점에 구도 함께 보입니다. 이 구가 대상 콜라이더에 닿지 않으면 콜라이더 크기나 위치를 먼저 조정합니다.

Blueprint 안에 `ChildActorComponent`나 별도 충돌 Actor를 쓰는 경우도 어느 정도 지원합니다. 실제로 맞은 Actor에 `JMInspectableComponent`가 없어도, 맞은 Actor의 Owner 또는 Attach Parent에 있는 `JMInspectableComponent`까지 찾아봅니다. 그래도 안 되면 Trace에 직접 맞는 Actor가 들어 있는 Blueprint에 `JMInspectableComponent`를 붙이는 구성이 가장 안정적입니다.

## Project Settings 위치

플러그인 설정은 다음 위치에 있습니다.

```text
Edit
-> Project Settings
-> Plugins
-> Item Inspector
```

주요 설정:

- `Default Trace Distance`: 상호작용 기본 거리
- `Default Trace Radius`: 상호작용 Trace의 기본 반경. `0`이면 Line Trace, `10 ~ 30`이면 작은 Sphere Trace처럼 동작합니다.
- `Interaction Trace Channel`: 라인트레이스 채널
- `Default Trace Mode`: 기본 탐색 방식
- `Trace Timer Interval`: Timer 모드일 때 탐색 간격
- `Draw Interaction Debug`: 디버그 라인 표시
- `Default Inspector Widget Class`: 기본 Inspector Widget Blueprint
- `Default Render Target Size`: Preview Render Target 해상도
- `Default Rotation Sensitivity`: 새 Inspection Data를 만들 때 참고할 회전 감도 기본값
- `Default Zoom Speed`: 마우스 휠 확대/축소 속도
- `Default Preview FOV`: Preview 카메라 시야각
- `Default Background Color`: Preview Render Target 기본 배경색. 기본값은 투명입니다.
- `Default Block Player Input`: Inspector를 열 때 플레이어 이동/시점 입력을 잠글지 여부
- `Default Pause Game`: Inspector를 열 때 게임 일시정지 여부
- `Duplicate Policy`: 이미 Inspector가 열려 있을 때 새 요청 처리 방식

`Default Inspector Widget Class`가 비어 있어도 괜찮습니다. 이 경우 플러그인이 제공하는 기본 C++ 위젯을 사용합니다.

## 커스텀 Inspector UI 만들기

기본 Inspector UI 대신 직접 만든 UI를 쓰고 싶다면 다음 순서로 진행합니다.

1. 콘텐츠 브라우저에서 Widget Blueprint를 만듭니다.
2. 부모 클래스를 `JMItemInspectionWidgetBase`로 지정합니다.
3. 원하는 UI를 디자인합니다.
4. 닫기 버튼을 만들었다면 클릭 이벤트에서 `RequestClose`를 호출합니다.
5. Project Settings의 `Default Inspector Widget Class`에 이 Widget Blueprint를 지정합니다.

기본 C++ 위젯은 임시 테스트용으로 충분하지만, 실제 게임에서는 이 방식으로 프로젝트별 UI를 만드는 것을 권장합니다.

커스텀 위젯에 어떤 TextBlock, Image, Button을 추가해야 하는지는 [WIDGET_SETUP.md](Guides/WIDGET_SETUP.md)에 자세히 정리되어 있습니다.

기본 C++ 위젯에서 자동으로 처리하는 항목:

- `DisplayName` 표시
- `DisplayCategory` 표시
- `Description` 표시
- `AdditionalInfo` 표시
- `ItemId` 표시
- Render Target 기반 Static Mesh Preview 표시
- Preview Mesh가 없을 때 안내 문구 표시
- Preview 영역 마우스 드래그 회전
- Preview 영역 마우스 휠 줌
- ESC 닫기
- `닫기 (ESC)` 버튼 닫기

## Inspector를 코드나 UI에서 직접 열기

월드 Actor와 상호작용하지 않고, 인벤토리 슬롯이나 퀘스트 UI에서 바로 Inspector를 열 수도 있습니다.

이때도 필요한 것은 `UJMItemInspectionData` 에셋입니다.

```cpp
if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
{
    if (UJMItemInspectionSubsystem* Subsystem = LocalPlayer->GetSubsystem<UJMItemInspectionSubsystem>())
    {
        Subsystem->OpenInspection(InspectionData);
    }
}
```

일시정지나 Source Actor 숨김 같은 옵션이 필요하면 요청 구조체를 사용합니다.

```cpp
FJMItemInspectionRequest Request;
Request.InspectionData = InspectionData;
Request.SourceActor = SourceActor;
Request.bPauseGame = true;
Request.bHideSourceActor = false;

Subsystem->OpenInspectionFromRequest(Request);
```

## 자주 헷갈리는 부분

### Inspection Data는 어디에 있나?

`Inspection Data`는 `UJMInspectableComponent`의 속성입니다.

찾는 순서:

```text
레벨에서 조사 대상 Actor 선택
-> Details 패널
-> 컴포넌트 목록에서 JMInspectableComponent 선택
-> JM Gameplay | Item Inspection 카테고리
-> Inspection Data 속성
```

여기에 `UJMItemInspectionData` 에셋을 넣습니다.

### UJMItemInspectionData는 어디서 만드나?

콘텐츠 브라우저에서 만듭니다.

```text
Content Browser
-> 빈 공간 우클릭
-> Miscellaneous
-> Data Asset
-> JMItemInspectionData 선택
```

### Static Mesh Actor를 배치하면 자동으로 조사되나?

아니요.

반드시 그 Actor에 `UJMInspectableComponent`를 추가하고 `Inspection Data`를 지정해야 합니다.

### 아이템을 획득하면 자동으로 Inspector가 열리나?

아니요.

Inspector는 "아이템을 화면에서 조사하는 기능"만 담당합니다. 획득, 인벤토리 추가, 퀘스트 진행은 게임별 시스템에서 처리해야 합니다.

### Preview Mesh를 넣었는데 화면에 Mesh가 안 보이면?

확인 순서:

```text
UJMItemInspectionData 에셋 열기
-> PreviewMesh에 Static Mesh가 지정되어 있는지 확인
-> 에셋 Save
-> 조사 대상 Actor의 Inspection Data가 이 에셋을 가리키는지 확인
-> Inspector를 다시 열기
```

Preview Mesh가 비어 있거나 로드에 실패하면 Inspector는 닫히지 않고 Preview 영역에 안내 문구를 표시합니다.

### Mesh 회전이나 줌이 안 되면?

마우스 입력은 Preview 영역 안에서만 처리됩니다.

- 왼쪽 정보 패널이나 닫기 버튼 위에서 드래그하면 Mesh가 회전하지 않습니다.
- 설명 스크롤 영역에서 휠을 움직이면 설명 스크롤이 우선될 수 있습니다.
- Preview 영역 위에 마우스를 올리고 왼쪽 드래그 또는 휠을 사용해야 합니다.

### Inspector에 이름이나 설명이 안 보이면?

확인 순서:

```text
UJMItemInspectionData 에셋 열기
-> DisplayName, Description 값 입력
-> 에셋 Save
-> 조사 대상 Actor 선택
-> UJMInspectableComponent 선택
-> Inspection Data에 방금 저장한 에셋이 들어 있는지 확인
```

`Inspection Data`에 다른 에셋이 들어 있거나 비어 있으면 원하는 정보가 표시되지 않습니다.

### 닫기 버튼이나 ESC가 동작하지 않으면?

기본 C++ 위젯에서는 ESC와 `닫기 (ESC)` 버튼이 이미 연결되어 있습니다.

커스텀 Widget Blueprint를 쓰는 경우에는 닫기 버튼 클릭 이벤트에서 반드시 `RequestClose`를 호출해야 합니다.

## 빌드 확인

마지막 확인 빌드는 성공했습니다.

```text
BaseUpgradeEditor Win64 Development
```

빌드 중 코드 오류는 없었고, 로컬 UnrealBuildTool 설정의 deprecated 경고만 출력되었습니다.
