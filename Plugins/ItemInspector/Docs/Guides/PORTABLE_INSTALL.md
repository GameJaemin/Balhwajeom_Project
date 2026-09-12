---
title: "다른 프로젝트에 이식하기"
status: ReviewRequired
authority: Guide
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "문서 구조만 정비했으며 현재 Editor 절차·에셋 경로·설정과 재검증하지 않음"
---

# 다른 프로젝트에 이식하기

이 문서는 `ItemInspector` 플러그인을 다른 Unreal Engine 프로젝트로 옮기는 방법을 설명합니다.

## 가장 쉬운 설치 방법

배포 ZIP을 받은 뒤, 대상 프로젝트 폴더에 압축을 풉니다.

대상 프로젝트 폴더 예시:

```text
MyProject
├─ MyProject.uproject
├─ Content
├─ Config
└─ Plugins
```

압축을 풀고 나면 다음 구조가 되면 됩니다.

```text
MyProject
└─ Plugins
   └─ ItemInspector
      ├─ ItemInspector.uplugin
      ├─ Source
      ├─ Content
      └─ Docs
```

그 다음 `.uproject`를 열고 플러그인을 활성화합니다.

```text
Edit
-> Plugins
-> Gameplay
-> Item Inspector
-> Enabled
```

활성화 후 에디터가 재시작을 요구하면 재시작합니다.

## 위젯 에셋

기본 Inspector Widget Blueprint는 플러그인 안에 들어 있습니다.

```text
Plugins
-> ItemInspector Content
-> UI
-> WBP_JMItemInspection
```

새 프로젝트에서 별도 설정을 하지 않으면 이 위젯을 기본으로 사용합니다.

만약 Content Browser에서 플러그인 콘텐츠가 보이지 않으면 Content Browser 오른쪽 위 `Settings`를 누르고 다음 옵션을 켭니다.

```text
Show Plugin Content
```

## Preview 투명 배경 머티리얼

Inspector Preview의 투명 배경 처리는 플러그인 안의 머티리얼이 담당합니다.

```text
Plugins
-> ItemInspector Content
-> ItemInspection
-> M_JMItemPreviewAlpha
```

이 에셋은 `WBP_JMItemInspection`과 기본 C++ 위젯에서 자동으로 사용합니다. 따로 지정하지 않아도 됩니다.

## 기존 프로젝트의 커스텀 Widget을 쓰고 싶을 때

대상 프로젝트에서 직접 만든 Widget Blueprint를 쓰려면 다음 위치에 지정합니다.

```text
Edit
-> Project Settings
-> Plugins
-> Item Inspector
-> Item Inspection
-> Default Inspector Widget Class
```

직접 만든 위젯은 부모 클래스를 `JMItemInspectionWidgetBase`로 지정해야 합니다.

## 샘플 아이템 에셋

배포 패키지에 `Content/Item` 폴더가 함께 들어 있다면, 이것은 테스트용 샘플입니다.

```text
Content
└─ Item
   ├─ WBP_JMItemInspection
   ├─ BP_Item
   ├─ DA_OldKey_Inspection
   └─ SM_OldKey
```

샘플 아이템까지 그대로 쓰고 싶으면 `Content/Item` 폴더도 대상 프로젝트의 `Content` 안에 복사합니다.

단, 실제 재사용에 필요한 기본 Inspector 위젯은 이미 플러그인 안에 있으므로 샘플 아이템 폴더는 필수가 아닙니다.

## 플레이어 Blueprint에서 해야 할 일

플러그인은 플레이어 입력을 자동으로 만들지 않습니다. 대상 프로젝트의 Player Character 또는 Pawn Blueprint에 `JMInteractionComponent`를 추가하고, 상호작용 입력에서 다음 함수를 호출합니다.

```text
Refresh Current Interactable
Try Begin Interaction
```

처음 테스트할 때 추천값:

```text
Trace Distance: 500
Trace Radius: 20
Trace Channel: Visibility
Trace Mode: OnInput
Draw Debug: true
```

## 조사 대상 Actor에서 해야 할 일

조사할 Actor 또는 Blueprint에 `JMInspectableComponent`를 추가합니다.

그 다음 `Inspection Data`에 `UJMItemInspectionData` 에셋을 지정합니다.

대상 Mesh나 Collision Component는 플레이어의 `Trace Channel`을 막아야 합니다.

```text
Collision Enabled: Query Only 또는 Query and Physics
Trace Responses -> Visibility: Block
```

## C++ 프로젝트에서 빌드가 필요할 때

대상 프로젝트가 C++ 프로젝트라면 에디터를 열 때 자동으로 빌드를 요구할 수 있습니다.

빌드가 실패하면 Visual Studio 2022와 해당 Unreal Engine 버전용 C++ 툴체인이 설치되어 있는지 확인합니다.

## Blueprint 전용 프로젝트에서 사용할 때

플러그인 배포본에 `Binaries` 폴더가 포함되어 있고, 대상 프로젝트의 Unreal Engine 버전이 동일하면 바로 열릴 수 있습니다.

엔진 버전이 다르면 다시 빌드가 필요할 수 있습니다. 이 경우 C++ 툴체인이 필요합니다.
