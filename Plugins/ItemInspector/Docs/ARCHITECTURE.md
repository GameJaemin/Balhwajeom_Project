---
title: "아키텍처"
status: ReviewRequired
authority: Canonical
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "2026-08-19 전체 감사 이후 문서 구조만 정비했으며 모든 세부를 현재 코드와 재대조하지 않음"
---

# 아키텍처

`ItemInspector`는 특정 프로젝트의 Player, HUD, GameMode, Inventory, Level Blueprint에 의존하지 않는 Runtime Plugin입니다.

게임 프로젝트는 다음 방식으로 플러그인과 연결합니다.

- Actor Component
- Unreal Interface
- Local Player Subsystem
- Data Asset
- Developer Settings
- Delegate
- Soft Reference

## Entrance transition

The existing `SceneCaptureComponent2D -> TextureRenderTarget2D -> UMG` preview pipeline remains intact. An optional transient full-viewport widget displays the same Render Target at the projected source-actor bounds and interpolates it into the inspector preview panel.

`UJMItemInspectionSubsystem` owns the transition and registers a Core Ticker only while entering, so it continues while gameplay is paused. `AJMItemInspectionPreviewActor` simultaneously interpolates toward the Data Asset's `InitialRotation`, `PreviewOffset`, `PreviewScale`, and `InitialZoom`. Completion reapplies `ApplyViewSettings()` for an exact authored end pose.

Configuration resolves as `request override -> Data Asset override -> project default`. Missing, invalid, or off-screen sources use the immediate-open path. A normal world inspection can enter `TransitioningOut`, interpolate the panel image and current user-adjusted preview pose back to the source, then broadcast `OnInspectionClosed`. Inventory inspections without a source close immediately, while inspect-before-pickup requests explicitly suppress the reverse transition.

## 모듈 구조

게임 실행용 Runtime 모듈과 Editor 전용 자동화 테스트 모듈을 사용합니다.

```text
Plugins/ItemInspector
└─ Source
   ├─ ItemInspectorRuntime
      ├─ Public
      │  ├─ Core
      │  ├─ Interaction
      │  └─ ItemInspection
      └─ Private
         ├─ Core
         ├─ Interaction
         └─ ItemInspection
   └─ ItemInspectorTests
      └─ Private/Tests
```

## 의존성 방향

```text
Core
-> Interaction
-> ItemInspection Bridge

ItemInspection
-> Core Settings / Log
```

`Core`는 공통 타입과 설정을 제공합니다.  
`Interaction`은 상호작용 탐색과 실행을 담당합니다.  
`ItemInspection`은 명시적으로 설정된 대상만 검사 화면을 열 수 있게 합니다.

## 상호작용과 조사 연결

`UJMInteractionComponent`는 라인트레이스로 Actor를 찾습니다.

대상 Actor가 `IJMInteractableInterface`를 구현했거나, 대상 Actor의 컴포넌트 중 `IJMInteractableInterface`를 구현한 컴포넌트가 있으면 상호작용 대상으로 처리합니다.

`UJMInspectableComponent`는 이 구조를 이용해 어떤 Actor에도 추가 가능한 조사용 Bridge Component로 동작합니다.

## 회전 가능한 표면 UMG

`UJMItemInspectionData.SurfaceWidget`은 노트, 카드, 사진, 전자 화면처럼 Preview Mesh 표면에 동적 UMG 콘텐츠를 표시하는 선택 기능입니다.

`AJMItemInspectionPreviewActor`의 `SurfaceWidgetComponent`는 `PreviewPivot`에 연결됩니다. 따라서 Mesh와 같은 회전 입력을 공유하며 Inspector 세션이 닫히면 Preview Actor와 함께 제거됩니다. ItemInspector는 노트 도메인을 알지 않고 `FText + Widget Class + Transform`만 처리합니다.

Widget Class가 비어 있으면 네이티브 `UJMItemInspectionSurfaceWidgetBase`를 사용하고, 로드에 실패해도 기존 Mesh Preview는 유지합니다. 별도 JM 플러그인 의존성은 추가되지 않습니다.

## Subsystem 선택

`UJMItemInspectionSubsystem`은 `ULocalPlayerSubsystem`을 사용합니다.

Inspector는 로컬 UI, 마우스 커서, 입력 포커스, 일시정지 상태와 관련된 기능이므로 GameInstance 전체가 아니라 Local Player 단위에서 관리하는 것이 자연스럽습니다.

## 멀티플레이 관점

Inspector UI는 로컬 Cosmetic 기능입니다.

- Preview 상태는 Replicate하지 않습니다.
- Inspector를 열었다고 서버 상태가 자동 변경되지 않습니다.
- 아이템 획득이나 인벤토리 추가는 Inspector가 담당하지 않습니다.
- 서버 권한 검사가 필요한 게임에서는 Interaction 또는 외부 게임 시스템에서 처리해야 합니다.
