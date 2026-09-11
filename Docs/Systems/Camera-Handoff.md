# 카메라 시스템 담당 작업 문서

> 기준 문서: [Project-Progress-and-Roadmap.md](../Project-Progress-and-Roadmap.md) 2.3(카메라와 사진), 5장 P1-2/P1-3
> 담당 범위: `UBalhwajeomPhotoCameraComponent`, `ABalhwajeomEvidenceActor`의 카메라 타깃 인터페이스, `BalhwajeomEvidenceCameraHUD`, `BalhwajeomFixedCameraZone`

이 문서는 **카메라 세부 시스템 수정**을 맡은 담당자용 문서다. 카메라의 기본 촬영 루프(중앙 레이→거리 판정→상태 검증→저장)는 프로토타입 완료 상태이며, 이번 일정에서는 실제 실행 환경 검증과 세부 동작 다듬기가 핵심이다.

## 1. 현재 구조 (변경 대상 아님)

핵심 파일: [BalhwajeomPhotoCameraComponent.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h) / `.cpp`, [BalhwajeomEvidenceActor.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceActor.h), [BalhwajeomEvidenceCameraHUD.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceCameraHUD.h), [BalhwajeomFixedCameraZone.h](../../Source/Balhwajeom/CameraSystem/BalhwajeomFixedCameraZone.h)

동작 개요:
- `UBalhwajeomPhotoCameraComponent`가 공통 초점/촬영 범위와 블러 규칙을 소유한다. 기본 범위는 `MinimumFocusDistance=400cm`부터 `MaximumFocusDistance=1000cm`까지다.
- 뷰포트 정중앙의 단일 `ECC_Visibility` 레이가 처음 맞힌 대상만 후보가 된다. 촬영 가능 여부는 카메라에서 그 대상의 `CameraFocusPoint`까지의 실제 직선거리로 판정한다.
- `FEvidenceStateDefinition`의 `MinimumFocusDistanceOffset`과 `MaximumFocusDistanceOffset`이 대상별 범위를 조절한다. 두 값의 기본값은 0이며, 유효 최소값은 0cm 아래로 내려가지 않는다.
- 줌/FOV는 화면 확대에만 관여한다. 초점 거리, 촬영 가능 거리, 블러 구간은 바꾸지 않는다.
- 화면 가이드는 `CameraFocusPoint`의 투영 위치에 표시한다. 엄격 초점을 잃어도 `FocusTargetGracePeriod`(기본 0.1초) 동안 시각 초점만 유지되며, 이 유예 상태로는 촬영할 수 없다.
- 기존 실루엣 후보 탐색과 70% 프레이밍 커버리지는 촬영 판정에서 사용하지 않는다.
- 화면 블러는 깊이 버퍼와 좌표계를 맞추기 위해 `CameraFocusPoint`의 카메라 전방축 깊이를 사용한다. 초점 대상이 있으면 이 깊이의 앞뒤 `BlurStartDistance`까지 선명하다. 대상이 없으면 전역 최소~최대 범위 전체가 선명하다. 바깥은 `BlurTransitionDistance` 동안 2차 Ease-In으로 `MaximumBlurStrength`까지 흐려진다.
- 블러는 원거리 가로/세로, 근거리 가로/세로, 최종 합성의 전체 해상도 5패스로 처리한다. 17샘플 가우시안 커널이 텍스처뿐 아니라 흐린 물체의 외곽선도 연속적으로 퍼뜨린다. 근거리 패스는 색과 커버리지를 함께 누적한 뒤 원거리 결과 위에 합성하므로 앞쪽 물체가 뒤쪽 물체를 자연스럽게 덮는다. 모든 색상/깊이 샘플은 같은 Viewport UV에서 출발해 각 텍스처 좌표로 따로 변환·고정하므로 중간 렌더 타깃 크기 차이로 인한 위치 밀림이 없다. 카메라 HUD/WBP는 장면 후처리 뒤에 그려져 선명하게 유지된다.
- 일반 불투명/마스크드 메시에는 별도의 메시 머티리얼 수정 없이 자동 적용된다. 단, Translucent 및 Separate Translucency 렌더링은 장면 깊이/후처리 순서 특성상 동일한 블러 결과를 보장하지 않는다. 해당 렌더링 방식은 별도 아트 대응이 필요하다.
- `TryCaptureActiveFocusTarget` → `BeginInvestigationImageCapture` → 뷰포트 스크린샷 → PNG 비동기 저장 → `UBalhwajeomInvestigationSubsystem::RegisterCapturedPhoto`.
- 같은 `PhotoID`는 `HasCapturedPhoto`로 중복 차단되며, 등록 실패 시 생성된 이미지 파일을 정리한다.
- `AddEvidence`/`HasEvidence`/`GetCollectedEvidence`는 Deprecated(레거시)로 표시되어 있고, `BalhwajeomInvestigationSubsystem` API로 대체됐다.

## 2. 디자이너 조정 위치

- 플레이어 카메라 Blueprint의 `BalhwajeomPhotoCameraComponent` Class Defaults:
  - `MinimumFocusDistance`, `MaximumFocusDistance`: 공통 초점/촬영 가능 범위
  - `BlurStartDistance`: 초점점 앞뒤의 완전 선명 범위
  - `BlurTransitionDistance`: 최대 블러에 도달하기까지의 거리
  - `MaximumBlurStrength`: 정규화된 최대 블러 강도(0~1)
  - `MaximumBlurRadiusPixels`: 1080p 기준으로 최대 세기 1.0이 표현할 전체 해상도 블러 반경
  - `NearBlurRadiusScale`: 앞쪽 물체의 블러 반경 배율. 기본값은 1.25
  - `FarBlurRadiusScale`: 뒤쪽 물체의 블러 반경 배율. 기본값은 1.0
  - `DepthRejectionDistance`: Blueprint 직렬화 호환을 위해 남겨 둔 Deprecated 값. 현재 5패스 블러에는 사용되지 않는다.
  - `FocusApplicationSpeed`: 초점과 선명 구간이 새 목표로 이동하는 속도
  - `FocusTargetGracePeriod`: 순간적인 중앙 레이 이탈을 숨기는 시각 유예 시간
- `DT_EvidenceStates` 각 행:
  - `MinimumFocusDistanceOffset`, `MaximumFocusDistanceOffset`: 해당 상태만의 거리 보정
- 증거 Blueprint/레벨 인스턴스:
  - `CameraFocusPoint`: 거리 측정과 가이드 표시의 정확한 기준점
- 후처리 에셋(기존 직렬화 참조 호환을 위해 에셋 경로는 유지):
  - `/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter`: 원거리 가로 패스, `FocusFarHorizontal` 출력, 우선순위 0
  - `/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur`: 원거리 세로 패스, `FocusFarBlurred` 출력, 우선순위 1
  - `/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal`: 근거리 색/커버리지 가로 패스, `FocusNearHorizontal` 출력, 우선순위 2
  - `/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical`: 근거리 색/커버리지 세로 패스, `FocusNearBlurred` 출력, 우선순위 3
  - `/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite`: 원본·원거리·근거리 결과의 최종 합성, 우선순위 4
  - 런타임 공통 파라미터는 `SharpNearDistance`, `SharpFarDistance`, `BlurTransitionDistance`, `MaximumBlurStrength`, `MaximumBlurRadiusPixels`, `NearBlurRadiusScale`, `FarBlurRadiusScale`다.
  - 새 불투명/마스크드 모델을 맵에 배치하는 것만으로 블러에는 자동 참여한다. 카메라 자동 초점/촬영 대상으로 사용하려면 별도로 증거 액터 구성과 `CameraFocusPoint`가 필요하다.

PIE에서는 먼저 FOV를 바꿔도 같은 실제 거리에서 초점 판정이 유지되는지 확인하고, 대상 앞을 다른 메시로 가렸을 때 뒤 대상이 잡히지 않는지 확인한다. 이어서 중앙에서 살짝 벗어났을 때 약 0.1초 뒤 가이드가 사라지는지, 그 짧은 유예 중 셔터가 촬영을 거절하는지 확인한다.

## 3. 변경 필요 사항 (세부 시스템 수정)

- [ ] **패키지 환경 검증**: PIE와 Development 패키지 양쪽에서 PNG 파일이 실제로 생성되는지 확인. `viewport screenshot` 방식이 패키지에서 실패하면 SceneCapture 방식으로 전환.
- [ ] 저장된 이미지에 HUD, 조준선, 카메라 연출(포커스 가이드 등)이 포함되지 않는지 확인 — 스크린샷 캡처 시점과 HUD 표시 시점 분리 여부 점검.
- [ ] 고해상도 PNG 압축 시 프레임 hitch(순간 끊김) 측정 및 필요 시 비동기 저장 큐 조정.
- [ ] `AddEvidence`/`HasEvidence`/`GetCollectedEvidence`/`CollectedEvidence` 등 Deprecated 필드의 Blueprint 참조를 Reference Viewer로 확인 후 참조 0건이면 제거.
- [ ] 카메라 HUD의 2D/3D 연출 방향 확정(로드맵 P2) — 이번 일정에서는 최소한 현재 방식 유지 여부만 결론 내고, 세부 연출은 아트/이펙트 트랙과 협의.
- [ ] `FixedCameraZone`과 `PhotoCameraComponent` 간 카메라 모드 전환(`OnCameraModeExited`, `OnCameraTransitionFinished`) 경계 케이스(전환 중 재진입 등) 점검.

## 4. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: 세부 시스템 수정 항목 중 로직 버그 위주로 우선 처리(초점/프레이밍/중복 차단 등), Deprecated API 참조 확인
- **2단계(9/13~9/15, M2)**: 패키지 빌드로 촬영 검증, hitch 측정, 아트/이펙트/사운드 트랙과 카메라 플래시·셔터음 연동 지점 확정
- **3단계(9/16~9/17, M3)**: Level_Main 통합 이후 발견되는 카메라 관련 버그 수정, 회귀 테스트
- **4단계(9/18, M4)**: 최종 QA 대응

## 5. 완료 조건

- 목표 플랫폼의 Development 패키지에서 촬영 품질과 성능 기준을 만족한다.
- Reference Viewer 기준 Deprecated 카메라 API의 Blueprint 참조가 0건이거나, 남아있다면 제거 계획이 기록되어 있다.
- 저장된 PNG에 HUD/조준선이 섞여 들어가지 않는다.

## 6. 주의 사항

### 촬영 후 월드 스토리 연출

촬영 파일 저장과 사진 등록이 모두 성공하면 `UBalhwajeomPhotoCameraComponent`가 `DT_Photos`의 `WorldStoryCues`와 `StoryVoice`를 읽어 `APhotoWorldStoryActor`를 생성한다.

- 셔터 요청 시 `Photo Story Screen X/Y Ratio`로 지정한 화면 지점을 월드로 Deproject하고 `Photo Story Display Distance`만큼 떨어진 위치의 Transform을 `FBalhwajeomPendingPhotoCapture`에 보관한다. 기본값은 가로 50%, 세로 72%, 거리 200cm다. 파일 저장은 비동기이므로 완료 시점의 카메라 Transform을 다시 사용하지 않는다.
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
