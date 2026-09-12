---
title: "TODO"
status: Draft
authority: Plan
scope: "Plugin: ItemInspector"
classified_on: 2026-08-19
owners:
  - "ItemInspector Maintainers"
review_reason: "기존 계획의 구현 완료 여부를 현재 코드와 대조하지 않음"
---

# TODO

## 완료된 Preview 작업

- Runtime Preview Actor 생성
- Preview StaticMeshComponent 생성
- SceneCapture2D와 Render Target 연결
- `UJMItemInspectionWidgetBase`의 Preview Image에 Render Target 표시
- Bounds 기반 카메라 거리 자동 계산
- Preview 영역 드래그로 Mesh 회전
- 마우스 휠 줌
- UI 버튼 클릭과 Preview 회전 입력 충돌 방지
- Preview 조명 프리셋과 커스텀 Key/Fill/Rim 조명 설정
- Preview 조명을 Directional Light 없이 Spot/Point Light로 구성
- Preview Lighting Channel 격리

## 다음 마일스톤

- Soft Reference 동기 로딩 안정화
- Async Load 확장 지점과 stale callback 방지
- Preview Actor 위치 정책 개선

## 이후 작업

- Demo Map 생성
- 샘플 Data Asset 생성
- Automation Test 추가
- Data Validation 추가
- 더 정교한 Input Mode 복구 정책
- 미래 Inventory 연동 예시 추가
- Package Build 검증
