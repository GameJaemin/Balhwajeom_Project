---
title: "아이템 조사 시스템"
status: ReviewRequired
authority: Supporting
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Public API·설정·런타임 계약과 재대조하지 않음"
---

# 아이템 조사 시스템

## 목적

아이템 조사 시스템은 개발자가 명시적으로 설정한 대상만 전용 검사 화면을 열 수 있게 하는 기능입니다.

이 시스템은 아이템 획득, 인벤토리, HUD, 특정 Player Character와 분리되어 있습니다.

## 주요 타입

- `UJMItemInspectionData`: 아이템 이름, 설명, Preview Mesh, View 설정을 담는 Data Asset
- `FJMItemInspectionRequest`: 월드 Actor, 인벤토리, 퀘스트 UI 등 다양한 호출자가 공통으로 쓰는 요청 구조체
- `UJMInspectableComponent`: 월드 Actor를 조사 가능 대상으로 만드는 컴포넌트
- `UJMItemInspectionSubsystem`: Local Player 단위 조사 세션 관리자
- `UJMItemInspectionWidgetBase`: 교체 가능한 UMG 기본 위젯

## 월드 상호작용 실행 흐름

```text
UJMInteractionComponent
-> 라인트레이스 대상 탐색
-> UJMInspectableComponent 감지
-> CompleteInteract
-> FJMItemInspectionRequest 생성
-> UJMItemInspectionSubsystem
-> UJMItemInspectionWidgetBase 생성
```

로컬 플레이어를 얻지 못하면 Inspector 열기는 실패합니다. AI나 서버 전용 Actor가 호출해도 Crash가 나지 않도록 설계했습니다.

## Inspector 직접 열기

월드 Actor 없이 Data Asset만으로 Inspector를 열 수 있습니다.

```cpp
Subsystem->OpenInspection(InspectionData);
```

옵션이 필요하면 `FJMItemInspectionRequest`를 사용합니다.

```cpp
FJMItemInspectionRequest Request;
Request.InspectionData = InspectionData;
Request.bPauseGame = true;
Request.bHideSourceActor = false;

Subsystem->OpenInspectionFromRequest(Request);
```

## Inspector 종료

종료 방식:

- ESC
- Close Button
- 외부 C++ 또는 Blueprint 호출
- Local Player Subsystem Deinitialize
- World Tear Down

종료 시 복구하는 상태:

- 생성한 Widget 제거
- 마우스 커서 표시 상태
- 플러그인이 적용한 Pause 상태
- 숨긴 Source Actor 표시 상태

## 기본 위젯에 표시되는 정보

`UJMItemInspectionWidgetBase` 기본 C++ 위젯은 다음 값을 표시합니다.

- `DisplayName`: 아이템 이름
- `DisplayCategory`: 아이템 분류
- `Description`: 본문 설명
- `AdditionalInfo`: 추가 정보
- `ItemId`: 디버그와 식별용 ID

`DisplayCategory`, `AdditionalInfo`, `ItemId`가 비어 있으면 해당 줄은 숨깁니다.

닫기는 ESC와 `닫기 (ESC)` 버튼을 지원합니다.

## Preview 구현

Inspector가 열리면 `UJMItemInspectionSubsystem`이 Runtime 전용 `AJMItemInspectionPreviewActor`를 스폰합니다.

Preview Actor는 다음 구성으로 동작합니다.

```text
AJMItemInspectionPreviewActor
├─ Preview StaticMeshComponent
├─ Surface WidgetComponent (선택)
├─ SceneCaptureComponent2D
├─ Spot Key Light
├─ Point Fill Light
└─ Point Rim Light
```

`SceneCaptureComponent2D`는 `UTextureRenderTarget2D`에 미리보기 결과를 기록하고, `UJMItemInspectionWidgetBase`는 그 Render Target을 Preview Image에 표시합니다.

현재 구현 방식은 Editor 전용 `FPreviewScene`을 쓰지 않습니다. Runtime 패키징을 고려해 게임 월드에 Preview Actor를 임시로 스폰하고, SceneCapture의 ShowOnly 목록으로 Preview Actor만 렌더링합니다.

Preview 전용 조명은 Directional Light를 사용하지 않습니다. 레벨의 Directional Light와 경쟁해서 `Multiple directional lights are competing...` 경고가 뜨는 것을 피하기 위해 Spot/Point Light 조합을 사용합니다.

기본값에서는 Preview Mesh, Preview Lights, SceneCapture View가 Lighting Channel 2를 사용합니다. 이 설정은 `UJMItemInspectionData -> ViewSettings -> LightingSettings -> Isolate From World Lighting`으로 조절할 수 있습니다.

Preview Mesh가 없거나 로드에 실패하면 Inspector는 계속 열리며 Preview 영역에 안내 문구를 표시합니다.

## Mesh 표면에 노트 글자 표시

`UJMItemInspectionData -> Surface Widget`을 활성화하면 World-space UMG가 Preview Mesh와 같은 Pivot에 생성됩니다.

- `Use Description As Content`: 켜면 Data Asset의 `Description`을 표면 본문으로 재사용합니다.
- `Widget Class`: `UJMItemInspectionSurfaceWidgetBase` 자식 WBP입니다. 비워두면 네이티브 기본 글자 위젯을 사용합니다.
- `Draw Size`: UMG 렌더 해상도입니다.
- `Surface Transform`: Mesh 로컬 공간의 위치, 회전, 크기입니다.
- `Two Sided`: 뒷면에서도 표면 위젯을 보일지 결정합니다.

기본 Paper 샘플은 `/ItemInspector/Item/Materials/WB_NoteMaster`와 `/ItemInspector/Item/DA_PaperInspection`을 연결합니다. `Description`만 바꾸면 Inspector의 종이 표면 글자도 바뀝니다. 회전과 줌을 허용한 상태에서도 글자는 Mesh와 함께 움직입니다.

## Preview 입력

기본 입력:

- Preview 영역에서 마우스 왼쪽 버튼 드래그: Mesh 회전
- Preview 영역에서 마우스 휠: 확대/축소
- ESC: Inspector 닫기
- `닫기 (ESC)` 버튼: Inspector 닫기

입력은 Preview 영역 안에서만 처리됩니다. 닫기 버튼이나 설명 영역 위에서 드래그해도 Mesh 회전으로 처리하지 않습니다.

## UI 교체

기본 C++ 위젯은 프로젝트별 WBP가 없어도 동작합니다.

게임별 UI가 필요하면 `UJMItemInspectionWidgetBase`를 상속한 Widget Blueprint를 만들고 Project Settings의 `Default Inspector Widget Class`에 지정합니다.

커스텀 닫기 버튼에서는 `RequestClose`를 호출하면 됩니다.

커스텀 Widget Blueprint에 필요한 정확한 위젯 이름과 타입은 [WIDGET_SETUP.md](../Guides/WIDGET_SETUP.md)에 정리되어 있습니다.

## 알려진 제한사항

- Preview Mesh 로딩은 현재 동기 로딩입니다. 큰 Mesh는 이후 Async Load로 확장할 예정입니다.
- Preview Actor는 Runtime 월드에 임시로 스폰되며 Inspector 종료 시 제거됩니다.
- Input Mode 전체 복구는 UE 공개 API 제한 때문에 이후 정책을 더 보강해야 합니다.
