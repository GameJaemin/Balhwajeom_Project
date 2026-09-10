# 카메라 시스템 담당 작업 문서

> 기준 문서: [Project-Progress-and-Roadmap.md](../Project-Progress-and-Roadmap.md) 2.3(카메라와 사진), 5장 P1-2/P1-3
> 담당 범위: `UBalhwajeomPhotoCameraComponent`, `ABalhwajeomEvidenceActor`의 카메라 타깃 인터페이스, `BalhwajeomEvidenceCameraHUD`, `BalhwajeomFixedCameraZone`

이 문서는 **카메라 세부 시스템 수정**을 맡은 담당자용 문서다. 카메라의 기본 촬영 루프(초점→중앙→프레이밍→저장)는 프로토타입 완료 상태이며, 이번 일정에서는 실제 실행 환경 검증과 세부 동작 다듬기가 핵심이다.

## 1. 현재 구조 (변경 대상 아님)

핵심 파일: [BalhwajeomPhotoCameraComponent.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h) / `.cpp`, [BalhwajeomEvidenceActor.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceActor.h), [BalhwajeomEvidenceCameraHUD.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceCameraHUD.h), [BalhwajeomFixedCameraZone.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomFixedCameraZone.h)

동작 개요:
- `UBalhwajeomPhotoCameraComponent`가 현재 초점 대상의 `bCanCapture`, `PhotoID`, 거리 조건(`PreferredFocusDistanceAt1x`, `FocusDistanceToleranceAt1x`)을 `ABalhwajeomEvidenceActor`(`IBalhwajeomCameraTargetInterface`)로부터 읽는다.
- 실루엣 트레이스로 포커스 가이드를 계산하고(`SilhouetteTracePixelStep`, `GuideFacingDotThreshold` 등), 프레이밍 커버리지(`MinimumCaptureCoverageRatio`)와 중앙 정렬을 함께 판정한다.
- `TryCaptureActiveFocusTarget` → `BeginInvestigationImageCapture` → 뷰포트 스크린샷 → PNG 비동기 저장 → `UBalhwajeomInvestigationSubsystem::RegisterCapturedPhoto`.
- 같은 `PhotoID`는 `HasCapturedPhoto`로 중복 차단되며, 등록 실패 시 생성된 이미지 파일을 정리한다.
- `AddEvidence`/`HasEvidence`/`GetCollectedEvidence`는 Deprecated(레거시)로 표시되어 있고, `BalhwajeomInvestigationSubsystem` API로 대체됐다.

## 2. 변경 필요 사항 (세부 시스템 수정)

- [ ] **패키지 환경 검증**: PIE와 Development 패키지 양쪽에서 PNG 파일이 실제로 생성되는지 확인. `viewport screenshot` 방식이 패키지에서 실패하면 SceneCapture 방식으로 전환.
- [ ] 저장된 이미지에 HUD, 조준선, 카메라 연출(포커스 가이드 등)이 포함되지 않는지 확인 — 스크린샷 캡처 시점과 HUD 표시 시점 분리 여부 점검.
- [ ] 고해상도 PNG 압축 시 프레임 hitch(순간 끊김) 측정 및 필요 시 비동기 저장 큐 조정.
- [ ] `AddEvidence`/`HasEvidence`/`GetCollectedEvidence`/`CollectedEvidence` 등 Deprecated 필드의 Blueprint 참조를 Reference Viewer로 확인 후 참조 0건이면 제거.
- [ ] 카메라 HUD의 2D/3D 연출 방향 확정(로드맵 P2) — 이번 일정에서는 최소한 현재 방식 유지 여부만 결론 내고, 세부 연출은 아트/이펙트 트랙과 협의.
- [ ] `FixedCameraZone`과 `PhotoCameraComponent` 간 카메라 모드 전환(`OnCameraModeExited`, `OnCameraTransitionFinished`) 경계 케이스(전환 중 재진입 등) 점검.

## 3. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: 세부 시스템 수정 항목 중 로직 버그 위주로 우선 처리(초점/프레이밍/중복 차단 등), Deprecated API 참조 확인
- **2단계(9/13~9/15, M2)**: 패키지 빌드로 촬영 검증, hitch 측정, 아트/이펙트/사운드 트랙과 카메라 플래시·셔터음 연동 지점 확정
- **3단계(9/16~9/17, M3)**: Level_Main 통합 이후 발견되는 카메라 관련 버그 수정, 회귀 테스트
- **4단계(9/18, M4)**: 최종 QA 대응

## 4. 완료 조건

- 목표 플랫폼의 Development 패키지에서 촬영 품질과 성능 기준을 만족한다.
- Reference Viewer 기준 Deprecated 카메라 API의 Blueprint 참조가 0건이거나, 남아있다면 제거 계획이 기록되어 있다.
- 저장된 PNG에 HUD/조준선이 섞여 들어가지 않는다.

## 5. 주의 사항

### 촬영 후 월드 스토리 연출

촬영 파일 저장과 사진 등록이 모두 성공하면 `UBalhwajeomPhotoCameraComponent`가 `DT_Photos`의 `WorldStoryCues`와 `StoryVoice`를 읽어 `APhotoWorldStoryActor`를 생성한다.

- 셔터 요청 시 화면 가로 50%, 세로 72% 지점을 월드로 Deproject하고 카메라 앞 200cm 위치의 Transform을 `FBalhwajeomPendingPhotoCapture`에 보관한다. 파일 저장은 비동기이므로 완료 시점의 카메라 Transform을 다시 사용하지 않는다.
- `APhotoWorldStoryActor`는 플레이어와 카메라에 Attach하지 않는다. `UWidgetComponent`와 비공간화 `UAudioComponent`만 자신의 Root에 Attach하므로 생성 위치에 그대로 남는다.
- Cue 전환은 `StoryVoice` 시작 시각 기준으로 진행한다. 음성이 끝나면 마지막 문구가 페이드아웃하고 액터가 스스로 제거된다.
- 새 사진 스토리가 시작될 때 이전 스토리가 아직 재생 중이면 이전 음성과 Cue를 중단하고 페이드아웃한다.
- 기본 연출 액터는 `/Game/Balhwajeom/UI/PhotoStory/WBP_PhotoWorldStory`를 사용한다. Designer의 `StoryText` TextBlock에서 폰트, 색상, 그림자, 정렬을 직접 관리한다. 자동 줄바꿈은 런타임에서 비활성화되며 `DT_Photos.WorldStoryCues.Text`에 직접 입력한 개행만 표시된다.
- 사진 모드에서 3인칭으로 복귀하면 월드 위치와 Widget Component Scale은 유지한 채 `StoryText`의 실제 Font Size를 부드럽게 키운다. 확대 비율은 Class Defaults의 `Third Person Font Size Multiplier`(기본 1.8), 전환 시간은 `Third Person Font Size Transition Duration`(기본 0.25초)에서 조절한다. 렌더 타깃을 확대하지 않으므로 글자 해상도가 유지된다.

관련 코드:

- `CameraSystem/PhotoWorldStoryActor.h/.cpp`
- `CameraSystem/PhotoWorldStoryWidget.h/.cpp`
- `CameraSystem/BalhwajeomPhotoCameraComponent.h/.cpp`

- `.uasset`(카메라 Blueprint)과 `.umap` 수정은 상호작용/월드 트랙과 겹칠 수 있으므로 동시 편집 전 확인한다.
- 사진 등록·저장 관련 상태는 이 트랙에서 별도로 들고 있지 않고 항상 [Subsystem-Handoff.md](./Subsystem-Handoff.md)의 InvestigationSubsystem을 통해서만 다룬다.
