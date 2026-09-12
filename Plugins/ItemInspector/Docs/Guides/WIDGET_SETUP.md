---
title: "Inspector Widget 제작 및 이식 가이드"
status: ReviewRequired
authority: Guide
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# Inspector Widget 제작 및 이식 가이드

이 문서는 `ItemInspector`의 Item Inspector 화면을 직접 만들거나 다른 프로젝트로 옮길 때 필요한 위젯 구성을 설명합니다.

기능을 모르는 사람도 따라할 수 있도록, "어떤 위젯을 추가해야 하는지", "이름을 무엇으로 해야 하는지", "빠지면 어떤 기능이 안 되는지"를 기준으로 정리했습니다.

## 목표

Inspector Widget은 다음 역할을 합니다.

- 아이템 이름 표시
- 아이템 분류 표시
- 아이템 설명 표시
- 추가 설명 표시
- Item ID 표시
- Static Mesh Preview Render Target 표시
- Preview 영역에서 마우스 드래그로 Mesh 회전
- Preview 영역에서 마우스 휠로 확대/축소
- ESC 또는 닫기 버튼으로 Inspector 닫기

## 가장 쉬운 방법

기본 C++ 위젯을 그대로 쓰면 별도 Widget Blueprint를 만들지 않아도 됩니다.

Project Settings의 `Default Inspector Widget Class`가 비어 있으면 플러그인의 기본 C++ 위젯이 자동으로 사용됩니다.

```text
Edit
-> Project Settings
-> Plugins
-> Item Inspector
-> Default Inspector Widget Class 비워 두기
```

처음 테스트할 때는 이 방법이 가장 안전합니다.

## 커스텀 Widget Blueprint를 만드는 경우

게임에 맞는 UI 디자인을 쓰고 싶다면 Widget Blueprint를 만듭니다.

1. Content Browser에서 빈 공간을 우클릭합니다.
2. `User Interface -> Widget Blueprint`를 선택합니다.
3. 부모 클래스를 `JMItemInspectionWidgetBase`로 지정합니다.
4. 이름을 정합니다.
   예: `WBP_ItemInspector`
5. 위젯을 열고 아래 표에 있는 위젯들을 추가합니다.
6. 저장 후 Project Settings의 `Default Inspector Widget Class`에 `WBP_ItemInspector`를 지정합니다.

이미 만든 Widget Blueprint가 있다면 위젯을 열고 `Class Settings`에서 Parent Class를 `JMItemInspectionWidgetBase`로 바꿉니다.

## 자동 연결되는 위젯 이름

`JMItemInspectionWidgetBase`는 아래 이름을 가진 위젯을 자동으로 찾아서 사용합니다.

이름은 정확히 같아야 합니다. 대소문자도 맞춰야 합니다.

가능하면 각 위젯의 Details 패널에서 `Is Variable`도 켜 둡니다. 이름 기반 자동 연결과 Blueprint 내부 접근을 안정적으로 관리하기 좋습니다.

| 위젯 이름 | 타입 | 중요도 | 역할 |
|---|---|---:|---|
| `PreviewPanel` | `Border` | 필수 | Preview 영역 판정. 이 영역 안에서만 드래그 회전과 휠 줌이 동작합니다. |
| `PreviewImage` | `Image` | 필수 | SceneCapture2D가 만든 Render Target을 표시합니다. |
| `PreviewStatusText` | `TextBlock` | 권장 | Preview Mesh가 없거나 로드 실패했을 때 안내 문구를 표시합니다. |
| `ItemNameText` | `TextBlock` | 권장 | `DisplayName`을 표시합니다. |
| `CategoryText` | `TextBlock` | 선택 | `DisplayCategory`를 표시합니다. 값이 비면 숨겨집니다. |
| `DescriptionText` | `TextBlock` | 권장 | `Description`을 표시합니다. |
| `AdditionalInfoText` | `TextBlock` | 선택 | `AdditionalInfo`를 표시합니다. 값이 비면 숨겨집니다. |
| `ItemIdText` | `TextBlock` | 선택 | `ItemId`를 표시합니다. 값이 비면 숨겨집니다. |
| `CloseButton` | `Button` | 권장 | 클릭하면 Inspector를 닫습니다. |

## 최소 구성

정말 최소로만 만들려면 다음 4개를 추가합니다.

```text
PreviewPanel        Border
PreviewImage        Image
ItemNameText        TextBlock
CloseButton         Button
```

이 구성에서는 Preview 표시, 마우스 회전/줌, 아이템 이름, 닫기 버튼만 동작합니다.

설명, 분류, 추가 정보, ID를 보여주고 싶다면 아래 권장 구성을 사용합니다.

## 권장 구성

권장 레이아웃은 다음과 같습니다.

여기서 `Left Preview Area`, `Right Info Area`는 실제 위젯 이름이 아닙니다. "왼쪽에는 미리보기 영역을 만들고, 오른쪽에는 정보 표시 영역을 만든다"는 설명용 이름입니다.

실제로는 `HorizontalBox`, `SizeBox`, `Border`, `Overlay`, `VerticalBox` 같은 UMG 기본 위젯을 조합하면 됩니다.

```text
Root Overlay 또는 Canvas Panel
└─ MainRow (HorizontalBox)
   ├─ LeftContainer (SizeBox 또는 Border)
   │  └─ PreviewPanel (Border)
   │     └─ Overlay
   │        ├─ PreviewImage (Image)
   │        └─ PreviewStatusText (TextBlock)
   └─ RightContainer (SizeBox 또는 Border)
      └─ InfoPanel (VerticalBox)
         ├─ ItemNameText (TextBlock)
         ├─ CategoryText (TextBlock)
         ├─ ScrollBox
         │  └─ DescriptionText (TextBlock)
         ├─ AdditionalInfoText (TextBlock)
         ├─ ItemIdText (TextBlock)
         └─ CloseButton (Button)
            └─ TextBlock: "닫기 (ESC)"
```

더 간단히 만들고 싶다면 다음처럼 해도 됩니다.

```text
Canvas Panel
├─ PreviewPanel (Border)
│     └─ Overlay
│        ├─ PreviewImage (Image)
│        └─ PreviewStatusText (TextBlock)
├─ ItemNameText (TextBlock)
├─ CategoryText (TextBlock)
├─ ScrollBox
│  └─ DescriptionText (TextBlock)
├─ AdditionalInfoText (TextBlock)
├─ ItemIdText (TextBlock)
└─ CloseButton (Button)
   └─ TextBlock: "닫기 (ESC)"
```

`DescriptionText`는 긴 글을 처리해야 하므로 `ScrollBox` 안에 넣는 것을 권장합니다.

자동 연결에 중요한 이름은 `PreviewPanel`, `PreviewImage`, `PreviewStatusText`, `ItemNameText`, `CategoryText`, `DescriptionText`, `AdditionalInfoText`, `ItemIdText`, `CloseButton`입니다.

`MainRow`, `LeftContainer`, `RightContainer`, `InfoPanel` 같은 컨테이너 이름은 자유롭게 바꿔도 됩니다.

## 추천 위치와 크기

처음 만들 때는 1920x1080 화면을 기준으로 아래 값을 사용하면 무난합니다.

### 전체 화면 Root

Root는 `Overlay` 또는 `Canvas Panel`을 사용합니다.

권장:

```text
Anchors: Full Screen
Offset Left: 0
Offset Top: 0
Offset Right: 0
Offset Bottom: 0
```

배경을 어둡게 깔고 싶다면 Root 아래에 `Border`를 하나 추가합니다.

```text
Name: Backdrop
Type: Border
Anchors: Full Screen
Brush Color: R 0.02, G 0.02, B 0.025, A 0.92
```

`Backdrop`은 자동 연결되는 위젯이 아니므로 이름은 자유롭습니다.

### MainRow

`MainRow`는 왼쪽 Preview와 오른쪽 정보 패널을 가로로 나누는 컨테이너입니다.

권장:

```text
Type: HorizontalBox
Anchor: Full Screen
Padding: Left 64, Top 64, Right 64, Bottom 64
Horizontal Alignment: Fill
Vertical Alignment: Fill
```

Canvas Panel에 직접 올리는 경우:

```text
Anchors: Full Screen
Offset Left: 64
Offset Top: 64
Offset Right: -64
Offset Bottom: -64
```

### LeftContainer

`LeftContainer`는 Preview 영역을 담는 컨테이너입니다.

권장:

```text
Type: SizeBox 권장
HorizontalBox Slot Size: Fill
HorizontalBox Slot Value: 1.0
Padding Right: 32
Width Override: 비워 둠
Min Desired Width: 720
```

1920x1080 기준으로 왼쪽 Preview가 화면의 약 65% 정도를 차지하면 보기 좋습니다.

`Min Desired Width`가 보이지 않는다면 현재 선택한 위젯이 `SizeBox`가 아닐 가능성이 큽니다.

설정 방법:

```text
HorizontalBox 안에 SizeBox 추가
-> SizeBox 이름을 LeftContainer로 변경
-> SizeBox 안에 PreviewPanel 추가
-> LeftContainer(SizeBox)를 선택
-> Details
-> Child Layout 또는 SizeBox 섹션
-> Min Desired Width 체크
-> 값 720 입력
```

`Border`에는 `Min Desired Width`가 직접 보이지 않을 수 있습니다. 이 경우 `Border`를 `SizeBox` 안에 넣습니다.

```text
LeftContainer (SizeBox)
└─ PreviewPanel (Border)
```

### PreviewPanel

`PreviewPanel`은 반드시 `Border`로 만드는 것을 권장합니다.

이 위젯은 Preview 입력 판정에 사용되므로, 보이는 미리보기 영역 전체를 덮어야 합니다.

권장:

```text
Name: PreviewPanel
Type: Border
Horizontal Alignment: Fill
Vertical Alignment: Fill
Brush Color: R 0.0, G 0.0, B 0.0, A 0.0
Padding: 0
```

Preview Render Target은 프리뷰 전용 UI 머티리얼인 `/ItemInspector/ItemInspection/M_JMItemPreviewAlpha`를 통해 표시됩니다. 이 머티리얼은 SceneCapture의 알파를 UI용 투명도로 변환해서, 검은 배경 없이 아이템만 보이게 합니다.

따라서 아이템만 떠 있는 것처럼 보이게 하려면 `PreviewPanel`의 Brush Alpha를 `0`으로 두는 것을 권장합니다.

프리뷰 영역 뒤에 어두운 패널을 깔고 싶다면 `PreviewPanel` 자체가 아니라 그 바깥에 별도 `Border`를 하나 더 만들고 색을 넣습니다. 이렇게 하면 `PreviewPanel`은 계속 입력 판정만 담당하고, 배경 디자인은 따로 조절할 수 있습니다.

```text
LeftContainer (SizeBox)
└─ PreviewBackground (Border, 원하는 배경색)
   └─ PreviewPanel (Border, Brush Alpha 0)
      └─ Overlay
         ├─ PreviewImage
         └─ PreviewStatusText
```

`PreviewPanel` 자체에서 `Min Desired Width`, `Min Desired Height`가 보이지 않아도 괜찮습니다. 크기 제한은 바깥의 `LeftContainer (SizeBox)`에서 잡는 것을 권장합니다.

Preview 영역의 최소 높이까지 정하고 싶다면 이렇게 구성합니다.

```text
LeftContainer (SizeBox)
  Min Desired Width: 720
  Min Desired Height: 640
└─ PreviewPanel (Border)
```

Canvas Panel에 직접 배치하는 경우:

```text
Anchors: Left
Position X: 64
Position Y: 64
Size X: 1160
Size Y: 952
```

### PreviewImage

`PreviewImage`는 Render Target을 표시합니다.

권장:

```text
Name: PreviewImage
Type: Image
Horizontal Alignment: Fill
Vertical Alignment: Fill
Visibility: Visible
Brush: 비워 둬도 됨
```

`PreviewImage`는 `PreviewPanel` 안의 `Overlay`에 넣고 Fill로 채우는 방식을 추천합니다.

`PreviewImage` 뒤에 검은색, 하늘색, 회색 사각형이 계속 보인다면 먼저 `PreviewPanel`, `PreviewImage`의 부모 `Overlay`, 그 바깥 `Border`의 Brush Alpha가 `1`로 되어 있지 않은지 확인합니다. Render Target 자체는 하늘/월드 배경을 찍지 않고, 기본 위젯은 알파 변환 머티리얼을 자동으로 사용합니다.

```text
PreviewPanel
└─ Overlay
   └─ PreviewImage
```

### PreviewStatusText

Preview Mesh가 없거나 로드 실패했을 때 표시되는 안내 문구입니다.

권장:

```text
Name: PreviewStatusText
Type: TextBlock
Horizontal Alignment: Center
Vertical Alignment: Center
Padding: 32
Auto Wrap Text: true
Justification: Center
Color: 연한 회색
Font Size: 18 또는 20
```

`PreviewImage`와 같은 Overlay 안에 넣습니다.

```text
PreviewPanel
└─ Overlay
   ├─ PreviewImage
   └─ PreviewStatusText
```

### RightContainer

오른쪽 정보 패널을 담는 컨테이너입니다.

권장:

```text
Type: SizeBox 권장
HorizontalBox Slot Size: Fill
HorizontalBox Slot Value: 0.38
Min Desired Width: 420
Max Desired Width: 560
```

마찬가지로 `Min Desired Width`나 `Max Desired Width`가 안 보이면 `RightContainer`를 `SizeBox`로 만들어야 합니다.

권장 구조:

```text
RightContainer (SizeBox)
  Min Desired Width: 420
  Max Desired Width: 560
└─ InfoPanel (VerticalBox)
```

Canvas Panel에 직접 배치하는 경우:

```text
Anchors: Right
Position X: -584
Position Y: 64
Size X: 520
Size Y: 952
```

### InfoPanel

정보 텍스트들을 세로로 쌓는 컨테이너입니다.

권장:

```text
Type: VerticalBox
Padding: 0
Horizontal Alignment: Fill
Vertical Alignment: Fill
```

`RightContainer`가 Border라면 Border의 Padding을 주는 것이 편합니다.

```text
RightContainer Border Padding: 0 또는 24
InfoPanel Padding: 필요 없음
```

### ItemNameText

아이템 이름입니다.

권장:

```text
Name: ItemNameText
Type: TextBlock
Font Size: 30
Font Style: Bold
Auto Wrap Text: true
VerticalBox Slot: Auto
Padding Bottom: 8
```

### CategoryText

아이템 분류입니다.

권장:

```text
Name: CategoryText
Type: TextBlock
Font Size: 16
Color: 연한 파란색 또는 회색
Auto Wrap Text: true
VerticalBox Slot: Auto
Padding Bottom: 18
```

분류를 UI에 보이고 싶지 않으면 만들지 않아도 됩니다.

### Description ScrollBox

긴 설명을 담는 영역입니다.

권장:

```text
Type: ScrollBox
VerticalBox Slot Size: Fill
VerticalBox Slot Value: 1.0
Padding Bottom: 20
```

설명이 길어져도 닫기 버튼을 밀어내지 않게 하려면 ScrollBox Slot을 `Fill`로 두는 것이 좋습니다.

### DescriptionText

아이템 설명 본문입니다.

권장:

```text
Name: DescriptionText
Type: TextBlock
Font Size: 18
Auto Wrap Text: true
Line Height Percentage: 1.1 ~ 1.25
Color: 거의 흰색
```

### AdditionalInfoText

추가 정보입니다.

권장:

```text
Name: AdditionalInfoText
Type: TextBlock
Font Size: 15 또는 16
Auto Wrap Text: true
Color: 약간 흐린 회색
VerticalBox Slot: Auto
Padding Bottom: 12
```

### ItemIdText

개발 확인용 ID입니다.

권장:

```text
Name: ItemIdText
Type: TextBlock
Font Size: 12 또는 13
Color: 어두운 회색
Auto Wrap Text: true
VerticalBox Slot: Auto
Padding Bottom: 16
```

실제 게임에서 ID를 숨기고 싶다면 이 위젯은 만들지 않아도 됩니다.

### CloseButton

닫기 버튼입니다.

권장:

```text
Name: CloseButton
Type: Button
VerticalBox Slot: Auto
Height: 44 ~ 52
Horizontal Alignment: Fill
```

버튼 안에는 TextBlock을 넣습니다.

```text
Text: 닫기 (ESC)
Font Size: 16
Justification: Center
```

## 빠른 1920x1080 레이아웃 예시

Canvas Panel을 기준으로 바로 배치하려면 아래처럼 시작해도 됩니다.

```text
Root Canvas
├─ Backdrop
│  Anchors: Full Screen
│  Offsets: 0, 0, 0, 0
├─ PreviewPanel
│  Anchors: Top Left
│  Position: X 64, Y 64
│  Size: X 1160, Y 952
│  Child Overlay:
│     ├─ PreviewImage: Fill
│     └─ PreviewStatusText: Center, Padding 32
└─ InfoPanel
   Anchors: Top Right
   Position: X -584, Y 64
   Size: X 520, Y 952
   Children:
      ItemNameText
      CategoryText
      ScrollBox -> DescriptionText
      AdditionalInfoText
      ItemIdText
      CloseButton
```

다만 반응형 UI를 생각하면 `HorizontalBox + SizeBox` 구성이 더 관리하기 쉽습니다.

## Min Desired Width가 안 보일 때

`Min Desired Width`는 모든 위젯에 항상 보이는 값이 아닙니다.

대부분 다음 상황 중 하나입니다.

### 1. SizeBox가 아니라 Border, Image, TextBlock을 선택함

`Min Desired Width`는 `SizeBox`에서 설정하는 것이 가장 확실합니다.

해결:

```text
SizeBox를 추가
-> 크기 제한을 걸고 싶은 위젯을 SizeBox 안으로 넣기
-> SizeBox 선택
-> Min Desired Width 체크
-> 값 입력
```

### 2. Canvas Panel에 직접 배치 중임

Canvas Panel에 직접 배치할 때는 `Min Desired Width`보다 Slot의 `Size X`, `Size Y`가 더 중요합니다.

예:

```text
PreviewPanel 선택
-> Slot (Canvas Panel Slot)
-> Size X: 1160
-> Size Y: 952
```

이 경우 `Min Desired Width`를 찾지 않아도 됩니다.

### 3. HorizontalBox 안에 있는데 SizeBox 없이 넣음

HorizontalBox 안에서 비율로 나누고 싶다면 Slot의 Size를 씁니다.

```text
LeftContainer Slot
-> Size: Fill
-> Fill Value: 1.0

RightContainer Slot
-> Size: Fill
-> Fill Value: 0.38
```

크기 하한까지 필요하면 `HorizontalBox -> SizeBox -> 실제 내용` 구조로 만듭니다.

### 가장 추천하는 구조

```text
MainRow (HorizontalBox)
├─ LeftContainer (SizeBox)
│  Min Desired Width: 720
│  Min Desired Height: 640
│  Slot Size: Fill
│  Slot Fill Value: 1.0
│  └─ PreviewPanel (Border)
└─ RightContainer (SizeBox)
   Min Desired Width: 420
   Max Desired Width: 560
   Slot Size: Fill
   Slot Fill Value: 0.38
   └─ InfoPanel (VerticalBox)
```

## 화면 크기별 조정 팁

작은 화면에서는 오른쪽 정보 패널이 너무 넓으면 Preview 영역이 답답해집니다.

권장 비율:

```text
Preview 영역: 60 ~ 70%
정보 영역: 30 ~ 40%
```

최소 크기:

```text
PreviewPanel Min Width: 560
RightContainer Min Width: 360
전체 Padding: 32 이상
```

모바일이나 좁은 화면까지 고려한다면 세로 배치를 따로 만들거나, Preview 위 / 정보 아래 구조를 별도 WBP로 만드는 것이 좋습니다.

## 각 위젯을 배치할 때 주의할 점

위젯을 추가한 뒤에는 다음을 확인합니다.

```text
위젯 선택
-> Details 패널
-> 이름이 표와 정확히 같은지 확인
-> Is Variable 체크
```

특히 `PreviewPanel`, `PreviewImage`, `CloseButton`은 이름이 틀리면 기능이 눈에 띄게 빠집니다.

### PreviewPanel

`PreviewPanel`은 마우스 입력 판정에 사용됩니다.

Preview 영역에서만 Mesh 회전과 줌이 동작하게 하려면, 실제 미리보기 영역 전체를 `PreviewPanel`이 덮고 있어야 합니다.

주의:

- `PreviewPanel`이 너무 작으면 작은 영역에서만 회전합니다.
- `PreviewPanel` 이름이 다르면 회전/줌이 동작하지 않습니다.
- `PreviewPanel` 위에 Button 같은 입력 위젯을 덮으면 Preview 입력이 막힐 수 있습니다.
- 장식용 Overlay나 Text는 가능하면 `Hit Test Invisible`로 설정합니다.

### PreviewImage

`PreviewImage`에는 Render Target이 들어갑니다.

사용자가 직접 Texture를 넣지 않아도 됩니다. Inspector가 열릴 때 Subsystem이 Render Target을 만들어 자동으로 넣습니다.

주의:

- `PreviewImage` 이름이 다르면 Static Mesh Preview가 보이지 않습니다.
- Brush에 이미지를 직접 지정하지 않아도 됩니다.
- Size는 Preview 영역에 맞게 Fill로 두는 것을 권장합니다.

### PreviewStatusText

`PreviewStatusText`는 상태 안내용입니다.

예:

- `Preview Mesh가 지정되지 않았습니다.`
- `Preview Mesh를 로드하지 못했습니다.`
- `Static Mesh 미리보기를 준비하는 중입니다.`

이 위젯은 없어도 Crash는 나지 않지만, 없으면 사용자가 왜 Mesh가 안 보이는지 알기 어렵습니다.

### ItemNameText

`UJMItemInspectionData.DisplayName`이 표시됩니다.

비어 있으면 기본 문구로 `이름 없는 아이템`이 표시됩니다.

### CategoryText

`UJMItemInspectionData.DisplayCategory`가 표시됩니다.

예:

```text
열쇠
단서
문서
퀘스트 아이템
```

값이 비어 있으면 자동으로 숨겨집니다.

### DescriptionText

`UJMItemInspectionData.Description`이 표시됩니다.

긴 문장이 들어갈 수 있으므로 다음 설정을 권장합니다.

- Auto Wrap Text 켜기
- ScrollBox 안에 넣기
- 오른쪽 정보 패널 폭을 충분히 확보하기

### AdditionalInfoText

`UJMItemInspectionData.AdditionalInfo`가 표시됩니다.

짧은 보조 설명이나 힌트에 사용합니다.

예:

```text
손잡이에 2-B라고 적혀 있다.
뒷면에 작은 긁힌 자국이 있다.
```

값이 비어 있으면 자동으로 숨겨집니다.

### ItemIdText

`UJMItemInspectionData.ItemId`가 표시됩니다.

주로 디버그나 개발 확인용입니다. 실제 게임 UI에서 보이고 싶지 않다면 만들지 않아도 됩니다.

### CloseButton

`CloseButton`은 자동으로 닫기 기능에 연결됩니다.

버튼 안의 텍스트는 자유롭게 정해도 됩니다.

예:

```text
닫기
닫기 (ESC)
돌아가기
```

`CloseButton`이라는 이름의 Button을 만들면 클릭 시 자동으로 `RequestClose`가 호출됩니다.

다른 이름의 버튼을 쓰고 싶다면 직접 클릭 이벤트에서 `RequestClose`를 호출해야 합니다.

## Data Asset 값이 Widget으로 들어가는 흐름

```text
UJMItemInspectionData
-> UJMItemInspectionSubsystem
-> UJMItemInspectionWidgetBase
-> Widget Blueprint의 이름 맞는 위젯들
```

필드별 연결:

| Data Asset 필드 | Widget |
|---|---|
| `DisplayName` | `ItemNameText` |
| `DisplayCategory` | `CategoryText` |
| `Description` | `DescriptionText` |
| `AdditionalInfo` | `AdditionalInfoText` |
| `ItemId` | `ItemIdText` |
| `PreviewMesh` | Preview Actor -> Render Target -> `PreviewImage` |

## Preview 회전과 줌이 동작하는 흐름

```text
사용자가 PreviewPanel 위에서 마우스 드래그
-> UJMItemInspectionWidgetBase가 입력 감지
-> OnPreviewDragged 이벤트 발생
-> UJMItemInspectionSubsystem
-> AJMItemInspectionPreviewActor 회전
-> SceneCapture2D가 다시 캡처
-> PreviewImage에 표시된 Render Target 갱신
```

휠 줌도 같은 구조로 동작합니다.

```text
사용자가 PreviewPanel 위에서 마우스 휠
-> OnPreviewZoomed 이벤트 발생
-> Preview Actor 카메라 거리 조정
-> Render Target 갱신
```

## Project Settings에 Widget 지정하기

Widget Blueprint를 만들었다면 반드시 Project Settings에 지정해야 합니다.

```text
Edit
-> Project Settings
-> Plugins
-> Item Inspector
-> Default Inspector Widget Class
-> WBP_ItemInspector 선택
```

지정하지 않으면 커스텀 WBP가 아니라 기본 C++ 위젯이 사용됩니다.

## 이식 체크리스트

다른 프로젝트로 옮길 때 확인할 것:

- `ItemInspector` 플러그인이 `Plugins` 폴더에 있는가
- 플러그인이 활성화되어 있는가
- Widget Blueprint의 부모 클래스가 `JMItemInspectionWidgetBase`인가
- `Default Inspector Widget Class`에 커스텀 WBP를 지정했는가
- `PreviewPanel` 이름이 정확한가
- `PreviewImage` 이름이 정확한가
- `CloseButton` 이름이 정확한가
- `ItemNameText`, `DescriptionText` 이름이 정확한가
- 조사 대상 Actor의 `Inspection Data`에 `UJMItemInspectionData` 에셋이 들어 있는가
- `UJMItemInspectionData.PreviewMesh`에 Static Mesh가 들어 있는가

## 자주 생기는 문제

### Inspector는 열리는데 Mesh가 안 보임

확인할 것:

- `PreviewImage`가 있는가
- `PreviewImage` 이름이 정확한가
- `PreviewMesh`가 지정되어 있는가
- `PreviewStatusText`에 오류 안내가 보이는가

### Mesh 회전이나 줌이 안 됨

확인할 것:

- `PreviewPanel`이 있는가
- `PreviewPanel` 이름이 정확한가
- Preview 영역 위에서 드래그/휠을 하고 있는가
- `PreviewPanel` 위에 Button, ScrollBox 같은 입력 위젯이 덮여 있지 않은가
- `ViewSettings`에서 회전/줌 허용이 꺼져 있지 않은가

### 좌우 회전은 되는데 위아래 회전이 약함

확인할 것:

- `UJMItemInspectionData` 에셋을 엽니다.
- `ViewSettings`에서 `bAllowPitchRotation`이 켜져 있는지 확인합니다.
- `RotationSensitivity`가 너무 낮으면 `0.35`에서 `0.5` 사이로 올립니다.
- `PreviewPanel`을 드래그할 때 마우스 이동량이 작으면 회전도 작게 적용됩니다.

현재 C++ 프리뷰 액터는 Pitch/Yaw를 단순 각도로 제한하지 않고 누적 회전으로 처리합니다. 따라서 `bAllowPitchRotation`이 켜져 있다면 위아래 방향도 계속 회전할 수 있습니다.

### Preview에 하늘이나 밝은 배경이 보임

확인할 것:

- 커스텀 Widget Blueprint에서 `PreviewPanel`의 Brush Alpha가 `0`인지 확인합니다.
- `PreviewImage` 부모 위젯이나 바깥 `Border`가 밝은 불투명 색을 칠하고 있지 않은지 확인합니다.
- Project Settings의 `Default Background Color` Alpha가 `0`인지 확인합니다.
- 플러그인 콘텐츠에 `/ItemInspector/ItemInspection/M_JMItemPreviewAlpha` 머티리얼이 있는지 확인합니다.

프리뷰 렌더링은 월드의 하늘/구름/안개를 끄고 Preview Actor만 찍도록 구성되어 있습니다. 그래도 밝은 사각형이 보이면 대부분 UMG 위젯 쪽 배경색이 보이는 경우입니다.

### Preview에 검은 배경이 보임

확인할 것:

- `PreviewImage`를 직접 Texture Brush로 고정해두지 않았는지 확인합니다.
- 기본 C++ 위젯 또는 `JMItemInspectionWidgetBase`를 부모로 둔 Widget Blueprint를 사용하고 있는지 확인합니다.
- `/ItemInspector/ItemInspection/M_JMItemPreviewAlpha` 머티리얼이 삭제되지 않았는지 확인합니다.

이 머티리얼은 SceneCapture의 알파를 뒤집어 UI 투명도로 사용합니다. 머티리얼이 없으면 Render Target이 fallback으로 직접 표시되어 검은 배경처럼 보일 수 있습니다.

### 아이템마다 Preview 크기를 다르게 하고 싶음

`UJMItemInspectionData` 에셋을 열고 `ViewSettings -> InitialZoom`을 조정합니다.

권장 시작값:

- 작은 아이템: `1.3 ~ 1.8`
- 보통 아이템: `1.0`
- 큰 아이템: `0.7 ~ 1.0`

마우스 휠로 더 확대/축소할 수 있게 하려면 `MinZoom`, `MaxZoom` 범위도 같이 넓힙니다.

### 닫기 버튼이 안 됨

확인할 것:

- Button 이름이 `CloseButton`인가
- 다른 이름을 쓴다면 OnClicked에서 `RequestClose`를 직접 호출했는가

### 이름이나 설명이 안 나옴

확인할 것:

- `ItemNameText`, `DescriptionText` 이름이 정확한가
- Data Asset의 `DisplayName`, `Description`을 입력하고 저장했는가
- 조사 대상 Actor의 `Inspection Data`가 올바른 에셋을 가리키는가

## 완전 커스텀 UI를 만들고 싶다면

위젯 이름을 전부 다르게 쓰고 싶다면 기본 자동 바인딩을 사용할 수 없습니다.

이 경우 두 가지 중 하나를 선택합니다.

1. 위 표의 이름을 그대로 사용하고 디자인만 바꿉니다.
2. C++에서 `UJMItemInspectionWidgetBase`를 상속한 새 클래스를 만들고 `SetInspectionData`, `SetPreviewTexture`, `SetPreviewStatusText` 등의 함수를 직접 처리합니다.

대부분의 프로젝트에서는 1번 방식을 권장합니다.
