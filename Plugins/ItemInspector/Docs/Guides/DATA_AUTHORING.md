---
title: "데이터 작성법"
status: ReviewRequired
authority: Guide
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# 데이터 작성법

## Entrance transition authoring

The existing `ViewSettings` fields remain the final inspection pose:

- `InitialRotation`: final preview rotation
- `PreviewOffset`: final preview-local position
- `PreviewScale`: final mesh scale
- `InitialZoom`: final Scene Capture zoom

Project defaults live under **Project Settings > Plugins > Item Inspector > Item Inspection > Transition**. Enable `bOverrideTransitionSettings` on a Data Asset only when that item needs different duration, easing, fade timing, or source visibility behavior. `FJMItemInspectionRequest` can override both levels.

`UJMInspectableComponent` supplies `SourceActor` automatically. `bEnableExitTransition` and `ExitDuration` control returning a non-inventory world item to that source on close. Direct `OpenInspection(Data)` calls have no source and close immediately. Inspect-before-pickup integrations suppress the return automatically.

## Item Inspection Data

아이템 조사 화면에 필요한 데이터는 `UJMItemInspectionData` 에셋으로 관리합니다.

필드 역할:

- `ItemId`: 아이템을 구분하기 위한 안정적인 ID
- `DisplayName`: 화면에 표시할 이름
- `DisplayCategory`: 이름 아래에 표시할 분류
- `Description`: 긴 설명 텍스트
- `AdditionalInfo`: 설명 아래에 표시할 보조 정보
- `PreviewMesh`: Inspector 왼쪽 Preview 영역에 표시할 Static Mesh
- `MaterialOverrides`: Preview Mesh에 적용할 선택 Material Override
- `ViewSettings`: Preview 위치, 초기 회전, 스케일, 초기 확대값, 회전 허용 여부, 줌 범위, 조명 설정
- `SurfaceWidget`: Preview Mesh와 함께 회전하는 선택형 UMG 본문, 해상도와 로컬 Transform

## 월드 Actor를 조사 대상으로 만들기

1. Actor에 `UJMInspectableComponent`를 추가합니다.
2. `Inspection Data`에 `UJMItemInspectionData` 에셋을 지정합니다.
3. 필요하면 `PromptText`, `bBlockPlayerInputDuringInspection`, `bPauseGameDuringInspection`, `bHideSourceActorDuringInspection`을 조정합니다.

`Inspection Data`가 비어 있으면 Inspector는 열리지 않습니다.

## 데이터 작성 원칙

- 텍스트는 `FText`를 사용합니다.
- Static Mesh와 Material은 Soft Reference로 관리합니다.
- 모든 Static Mesh Actor를 자동 조사 대상으로 취급하지 않습니다.
- 아이템 획득 여부는 Inspector가 아니라 외부 게임 시스템이 결정합니다.

## 표시 정보 작성 예시

```text
ItemId: OldKey
DisplayName: 낡은 열쇠
DisplayCategory: 열쇠
Description: 오래된 교실 문을 열 수 있을 것 같은 열쇠다.
AdditionalInfo: 손잡이 부분에 2-B라고 적혀 있다.
```

기본 Inspector 위젯은 이 값을 오른쪽 정보 패널에 표시합니다.

## Preview 설정 작성 팁

- `PreviewMesh`가 비어 있으면 Inspector는 열리지만 Preview 영역에는 안내 문구가 표시됩니다.
- `InitialZoom`은 Inspector를 열었을 때 처음 보이는 크기입니다. 작은 아이템은 `1.3 ~ 1.8`, 큰 아이템은 `0.8 ~ 1.0`부터 테스트합니다.
- `PreviewScale`은 Mesh 자체의 스케일 보정값입니다. 화면에 보이는 크기를 조절하려면 먼저 `InitialZoom`을 사용합니다.
- `InitialRotation`은 Inspector를 열었을 때 처음 보이는 각도를 정합니다.
- `RotationSensitivity`는 마우스 드래그 회전 속도입니다.
- `bAllowPitchRotation`, `bAllowYawRotation`으로 회전 축을 제한할 수 있습니다.
- `bAllowZoom`, `MinZoom`, `MaxZoom`으로 휠 줌을 제어할 수 있습니다.
- `LightingSettings`는 Inspector Preview 전용 조명입니다. 레벨 조명과 분리된 Key/Fill/Rim 조명을 사용합니다.

예를 들어 열쇠처럼 작은 물체는 다음처럼 시작하면 보기 쉽습니다.

```text
InitialZoom: 1.5
MinZoom: 0.8
MaxZoom: 2.5
```

조각상처럼 큰 물체는 다음처럼 시작합니다.

```text
InitialZoom: 1.0
MinZoom: 0.6
MaxZoom: 1.8
```

## Preview 조명 설정 팁

`UJMItemInspectionData` 에셋을 열고 `ViewSettings -> Lighting Settings`를 펼칩니다.

처음에는 `Lighting Preset`만 바꿔보는 것을 권장합니다.

```text
SoftStudio      부드러운 기본 스튜디오 조명
BrightProduct   밝고 선명한 제품 사진 느낌
WarmDramatic    따뜻하고 대비가 강한 조명
CoolRim         차가운 림 라이트가 강한 조명
Flat            그림자가 적고 정보 확인이 쉬운 조명
```

값 조정 순서:

1. `Lighting Preset`을 먼저 고릅니다.
2. 전체가 어두우면 `Intensity Multiplier`를 `1.2 ~ 1.5`로 올립니다.
3. 전체가 너무 밝으면 `Intensity Multiplier`를 `0.7 ~ 0.9`로 낮춥니다.
4. 입체감이 부족하면 `CoolRim` 또는 `WarmDramatic`을 테스트합니다.
5. 더 세밀하게 조정하려면 `Override Preset`을 켜고 `Key Light`, `Fill Light`, `Rim Light` 값을 직접 바꿉니다.

성능 기본값:

- `Cast Preview Shadows`: 기본 꺼짐. 켜면 입체감은 좋아지지만 비용이 늘어납니다.
- `Affect Translucent Lighting`: 기본 꺼짐. 투명/반투명 재질 아이템을 정확히 보고 싶을 때만 켭니다.
- `Isolate From World Lighting`: 기본 켜짐. 레벨 조명이 Preview에 섞이지 않게 Lighting Channel 2를 사용합니다.

화면에 `Multiple directional lights are competing...` 경고가 보였다면 기존 Preview Directional Light가 레벨 Directional Light와 경쟁하던 상황입니다. 현재 구현은 Directional Light를 사용하지 않고 Spot/Point Light만 사용하므로 이 경고를 만들지 않습니다.
# 회전 가능한 노트 만들기

기존 Paper 샘플을 복제하는 방식이 가장 빠릅니다.

1. `/ItemInspector/Item/DA_PaperInspection`을 복제합니다.
2. `Item Id`, `Display Name`, `Description`을 새 노트에 맞게 바꿉니다.
3. `Surface Widget -> Enabled`를 켭니다.
4. `Use Description As Content`를 켭니다.
5. `Widget Class`는 `/ItemInspector/Item/Materials/WB_NoteMaster`를 사용합니다.
6. `View Settings -> Allow Pitch Rotation`, `Allow Yaw Rotation`, `Allow Zoom`을 필요에 맞게 설정합니다.
7. 월드 Paper Actor의 `JMInspectableComponent -> Inspection Data`에 새 Data Asset을 지정합니다.

종이 Mesh나 크기가 다르면 `Surface Transform`만 조절합니다. 노트 본문은 `Description` 한 곳에서만 작성하며 별도 Actor Widget에 다시 입력하지 않습니다.
