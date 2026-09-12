---
title: "구현 계획"
status: Draft
authority: Plan
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "기존 계획의 구현 완료 여부를 현재 코드와 대조하지 않음"
---

# 구현 계획

## 현재 프로젝트 기준 상태

- Unreal Engine 버전은 `BaseUpgrade.uproject` 기준 5.7입니다.
- C++ 프로젝트이며 Runtime 모듈 이름은 `BaseUpgrade`입니다.
- 기존 플러그인은 `VisualStudioTools`만 있었고, `ItemInspector`는 새로 생성했습니다.
- Enhanced Input은 `Config/DefaultInput.ini`에 설정되어 있고 프로젝트 모듈에도 포함되어 있습니다.
- Gameplay Tags와 UMG는 프로젝트 모듈이 아니라 새 플러그인 Runtime 모듈 의존성으로 추가했습니다.
- 플러그인 추가 전 기준 빌드는 `BaseUpgradeEditor Win64 Development`로 성공했습니다.

## 1차 마일스톤 범위

- `Plugins/ItemInspector` Runtime Plugin 생성
- `ItemInspectorRuntime` 모듈 생성
- 공통 로그 카테고리 생성
- 상호작용 Context, Prompt, Result 구조체 생성
- `UDeveloperSettings` 기반 Project Settings 생성
- `IJMInteractorInterface`, `IJMInteractableInterface` 생성
- `UJMInteractionComponent` 라인트레이스 탐색 및 명시적 실행 API 구현
- `UJMItemInspectionData`, `UJMInspectableComponent`, `UJMItemInspectionSubsystem`, `UJMItemInspectionWidgetBase` 생성
- 프로젝트별 WBP 없이도 열고 닫을 수 있는 기본 C++ Inspector Widget 제공

## 완료된 2차 Preview 작업

- Runtime Preview Actor
- SceneCapture2D
- Render Target
- Static Mesh 미리보기
- Bounds 기반 카메라 거리 자동 계산
- 마우스 드래그 회전
- 마우스 휠 줌
- Preview Mesh 누락/로드 실패 안전 처리

## 아직 미룬 항목

- Demo Map 및 샘플 Blueprint 에셋
- Automation Test
- Async Soft Reference 로딩
- 완전한 Input Mode 복구 정책

## 컴파일 검증 순서

1. 플러그인 descriptor와 Runtime 모듈 추가 후 빌드
2. Core와 Interaction 추가 후 빌드
3. Item Inspection 추가 후 빌드
4. UHT, 컴파일, 링크 오류 수정
5. Runtime Preview Actor와 Render Target 연결 후 빌드
6. 마우스 회전/줌 입력 연결 후 빌드

## 다음 마일스톤

다음 단계에서는 Demo Map, 샘플 Data Asset, Automation Test, Async Soft Reference 로딩을 추가합니다.
