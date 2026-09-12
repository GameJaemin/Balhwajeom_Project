---
title: "기존 프로젝트 분석 및 Migration 기록"
status: Archived
authority: Historical
scope: "Plugin: JMInteraction"
classified_on: 2026-08-19
owners:
  - "JMInteraction Maintainers"
archive_reason: "기존 프로젝트 분석 및 Migration 기록으로 현재 사용 지침과 분리"
---

# 기존 프로젝트 분석 및 Migration 기록

## 발견한 기존 구조

- Player: `/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter`
  - `JMInteractionComponent` 보유
  - E Key 이벤트에서 `TryBeginInteraction` 호출
  - 별도 호출 경로에서 `RefreshCurrentInteractable` 실행
- Enhanced Input: 이동/시점/점프와 Inventory는 Enhanced Input을 사용하지만 Interaction은 현재 E Key Blueprint 이벤트다.
- 공통 Interaction: 원래 `ItemInspectorRuntime/Public/Interaction`과 `Core/JMGameplayTypes.h`에 존재했다.
- Trace: 기존 `UJMInteractionComponent`가 Center view line/sphere trace를 수행했다. Cursor trace, Focus callback, 기본 Prompt widget은 없었다.
- Item: `AReuseInspectableInventoryPickup`만 인터페이스 어댑터였고 실제 획득은 `AInventoryWorldItemPickup::TryPickup -> UInventoryComponent::AddItemDetailed`였다.
- Dialogue: `UReuseDialogueInteractableComponent -> UDialogueSubsystem::StartDialogue`였다.
- Door: `JMDoorGameplayIntegration`의 WorldSubsystem이 `UJMDoorInteractableAdapterComponent`를 Door에 자동 추가하고 `UJMDoorComponent::ToggleDoor`를 호출했다.
- UI: Item inspection UI는 있었지만 독립적인 interaction prompt widget은 없었다.

## 수행한 Migration

- 기존 계약/컴포넌트를 독립 `JMInteraction` Runtime 모듈로 이동했다.
- 기존 API 이름과 include 경로를 유지해 소비 코드를 최소 변경했다.
- Blueprint 자산용 Core Redirect를 추가했다.
- Inventory 기본 월드 Pickup 자체가 `IJMInteractableInterface`를 구현하도록 보강했다.
- Dialogue와 Door adapter는 핵심 기능을 유지하고 JMInteraction 계약만 소비한다.
- Prompt 기본값을 Item/Door/NPC별 FText로 분리했다.
- Player Blueprint의 기존 E 입력과 호출 API는 삭제하거나 재작성하지 않았다.

## 수동 Smoke Test 체크리스트

- Item을 조준하고 Prompt/획득/잔여 수량을 확인
- Door를 조준하고 Open/Close 및 잠금 흐름 확인
- NPC를 조준하고 Dialogue 시작/반복 정책 확인
- 대상 변경 시 Focus/Prompt가 한 번만 전환되는지 확인
- 거리 밖, 비 Interactable, 파괴된 Actor에서 안전하게 실패하는지 확인
- Cursor mode에서 커서 Hit와 거리 제한 확인
