# 증거 조사 시스템 3인 구현 분배안

## 1. 전체 진행 구조

```text
담당자 1: 데이터 형식과 공통 API 제공
                    ↓
┌───────────────────┼───────────────────┐
│                   │                   │
│ 담당자 1          │ 담당자 2          │ 담당자 3
│ Subsystem 구현    │ Evidence/상호작용 │ 카메라/사진
│                   │                   │
└───────────────────┴───────────────────┘
                    ↓
             통합 및 데이터 이전
                    ↓
               기존 코드 제거
                    ↓
               SaveGame 구현
```

핵심 원칙은 `ABalhwajeomEvidenceActor`를 담당자 2만 수정하고, 담당자 3은 카메라 인터페이스를 통해서만 증거 액터와 통신하는 것이다.

## 2. 담당자 1: 데이터 구조와 Subsystem

### 역할

모든 기능이 공통으로 사용할 데이터 규격, DataTable 조회 구조 및 런타임 관리자를 구현한다.

### 담당 파일

```text
Source/Balhwajeom/Balhwajeom.Build.cs

Source/Balhwajeom/Public/Investigation/
├─ EvidenceDefinitions.h
├─ PhotoDefinitions.h
├─ WordDefinitions.h
├─ SentenceDefinitions.h
├─ InvestigationRuntimeTypes.h
├─ BalhwajeomInvestigationSettings.h
└─ BalhwajeomInvestigationSubsystem.h

Source/Balhwajeom/Private/Investigation/
├─ BalhwajeomInvestigationSettings.cpp
├─ BalhwajeomInvestigationSubsystem.cpp
└─ Test/
```

### 정적 데이터 구조

- `FEvidenceDefinition`
- `FEvidenceStateDefinition`
- `FWordDefinition`
- `FPhotoDefinition`
- `FKeywordDocumentDefinition`
- `FKeywordChoiceDefinition`
- `FSentenceDefinition`
- `FSentenceWordSlot`
- `FSentencePhotoSlot`
- `FOutputTextDefinition`
- 관련 enum
- `PhotoDefinition.CaptureSound`

`FEvidenceStateDefinition`은 Excel/CSV 작업을 위해 상호작용·거리 표시·촬영 필드를 중첩하지 않고 평면 열로 관리한다.

### Settings 및 DataTable 구현

- `UBalhwajeomInvestigationSettings`
- Project Settings에서 7개 DataTable 참조
- ID 기반 DataTable 행 조회
- Row Name과 내부 ID 검증
- ID 참조 대상 존재 여부 검증

### Subsystem 구현

- 증거 액터 등록
- 증거 인스턴스별 현재 상태 관리
- `Once` 상호작용 완료 상태 관리
- 상태 변경
- 키워드 획득 및 중복 방지
- 촬영 사진 등록 및 중복 방지
- 문장 정답 판정
- 공통 이벤트 발생

### Subsystem 이벤트

```text
OnEvidenceStateChanged
OnWordAcquired
OnPhotoCaptured
OnSentenceSolved
```

### 테스트 범위

- 존재하지 않는 ID 조회 실패
- 중복 키워드 획득 차단
- 중복 `PhotoID` 등록 차단
- 잘못된 `ObjectID`의 상태 전환 차단
- `Once` 상호작용 완료 판정
- 키워드 및 사진 문장 정답 판정

### 담당하지 않는 영역

- `ABalhwajeomEvidenceActor` 수정
- 카메라 코드 수정
- 증거 블루프린트 메시 및 애니메이션
- 태블릿 UI 구현

### 완료 조건

다른 담당자가 다음 공통 API를 호출할 수 있어야 한다.

```text
RegisterEvidenceActor
GetEvidenceDefinition
GetEvidenceStateDefinition
BeginEvidenceInteraction
CompleteEvidenceInteraction
AcquireWord
HasCapturedPhoto
RegisterCapturedPhoto
ValidateSentence
```

## 3. 담당자 2: EvidenceActor와 3인칭 상호작용

### 역할

맵에 배치된 증거 오브젝트와 기존 거리 시스템 및 F 상호작용을 새 데이터 구조에 연결한다.

### 담당 파일

```text
Source/Balhwajeom/CameraSystem/
├─ BalhwajeomEvidenceActor.h
└─ BalhwajeomEvidenceActor.cpp

Source/Balhwajeom/Public/Interaction/
├─ InspectionComponent.h
├─ PlayerInteractionComponent.h
└─ PlayerInteractionTypes.h

Source/Balhwajeom/Private/Interaction/
├─ InspectionComponent.cpp
├─ PlayerInteractionComponent.cpp
└─ Test/PlayerInteractionDistanceStateTest.cpp
```

### EvidenceActor 변경

다음 식별 및 상태 필드를 추가한다.

```text
EvidenceInstanceID
ObjectID
CurrentStateID
```

다음 초기화 흐름을 연결한다.

```text
BeginPlay
→ Subsystem.RegisterEvidenceActor
→ ObjectID로 EvidenceDefinition 조회
→ CurrentStateID 결정
→ EvidenceStateDefinition 적용
```

### 상태 변화 처리

```text
OnEvidenceStateChanged
→ Actor의 CurrentStateID 갱신
→ 거리 UI 갱신
→ Blueprint 상태 변경 이벤트 발생
```

블루프린트 이벤트에는 이전 상태와 새로운 상태를 전달한다.

```text
OnEvidenceStateChanged(PreviousStateID, NewStateID)
```

블루프린트는 이 이벤트를 이용해 메시, 머티리얼, 애니메이션 및 충돌을 변경한다.

### InspectionComponent 변경

유지하는 기능:

- `CloseDistance`
- `MiddleDistance`
- `MaxDisplayDistance`
- 거리 판정 이벤트

DataTable로 이전하는 데이터:

```text
FarLabel
MidLabel
NearLabel
InspectionText
```

거리별 문구는 현재 `EvidenceStateDefinition`에서 가져온다.

### PlayerInteractionComponent 변경

기존 흐름:

```text
F 입력
→ InspectionComponent.InspectionText 반환
```

변경 흐름:

```text
F 입력
→ 바라보는 EvidenceActor 확인
→ Subsystem.BeginEvidenceInteraction
→ InteractionPresentation에 따른 UI 실행
→ 성공 후 CompleteEvidenceInteraction
```

구현할 상호작용:

- `None`
- `Once`
- `Repeatable`
- `ChangeState`
- `SimpleText`
- `KeywordSelectionWindow`

### 담당 블루프린트 및 UI

```text
BP_EvidenceBase
BP_Evidence_A
BP_Evidence_B
BP_Evidence_C
WBP_ObjectLabel
WBP_InspectionMessage
```

블루프린트에는 다음 설정만 남긴다.

- `ObjectID`
- 메시와 충돌
- 거리 수치
- 라벨 위치 보정
- 카메라 초점 위치
- 상태 변경에 따른 메시 및 애니메이션 처리

### 담당하지 않는 영역

- `InvestigationSubsystem` 내부 구현
- `PhotoCameraComponent` 수정
- 카메라 HUD 수정
- 사진 수집 컨테이너 구현

### 완료 조건

- 상태별 거리 문구가 정상적으로 표시됨
- `Once`가 같은 상태에서 한 번만 실행됨
- `Repeatable`이 반복 실행됨
- `ChangeState` 이후 상태와 블루프린트 외형이 변경됨
- 키워드 창에서 공통 `AcquireWord` 함수가 호출됨

## 4. 담당자 3: 카메라와 사진 시스템

### 역할

기존 카메라 기능을 `PhotoID`와 `InvestigationSubsystem` 기반 촬영 구조로 전환한다.

### 담당 파일

```text
Source/Balhwajeom/CameraSystem/
├─ BalhwajeomCameraTargetInterface.h
├─ BalhwajeomEvidenceTypes.h
├─ BalhwajeomPhotoCameraComponent.h
├─ BalhwajeomPhotoCameraComponent.cpp
├─ BalhwajeomEvidenceCameraHUD.h
├─ BalhwajeomEvidenceCameraHUD.cpp
├─ BalhwajeomCameraCharacter.h
└─ BalhwajeomCameraCharacter.cpp
```

`BalhwajeomEvidenceActor.h/.cpp`는 담당자 2만 수정한다. 카메라 인터페이스의 변경 계약은 담당자 3이 전달하고, EvidenceActor의 실제 인터페이스 구현은 담당자 2가 작성한다.

### 카메라 Target 정보 변경

현재 정보:

```text
EvidenceData
EvidenceID
bAlreadyCollected
```

변경 정보:

```text
EvidenceInstanceID
ObjectID
StateID
PhotoID
bCanCapture
PreferredFocusDistance
FocusDistanceTolerance
bScaleFocusDistanceWithZoom
```

### 촬영 수집 방식 변경

제거 대상:

```text
CollectedEvidence
CapturedFocusTargets
AddEvidence
HasEvidence
MarkAsCollected 호출
```

새 흐름:

```text
촬영 성공
→ CurrentState.PhotoID 확인
→ Subsystem.HasCapturedPhoto(PhotoID)
→ CapturedPhotoRecord 생성
→ Subsystem.RegisterCapturedPhoto
```

### 유지할 카메라 기능

- 카메라 모드 진입 및 종료
- WASD 이동
- 마우스 회전
- 줌
- 초점 거리 판정
- 실루엣 위치 판정
- 화면 중앙 판정
- 대상 화면 포함 비율 70% 판정
- `? / ✓` 연출
- 촬영 플래시
- 사진 저장 애니메이션

### HUD 변경

기존 촬영 표시:

```text
EvidenceData.bAlreadyCollected
→ ? 또는 ✓
```

변경 촬영 표시:

```text
Subsystem.HasCapturedPhoto(PhotoID)
→ ? 또는 ✓
```

기존 중앙 문구:

```text
InspectionComponent.NearLabel
```

변경 중앙 문구:

```text
CurrentState.ObservationText
```

### 사진별 촬영 사운드

```text
촬영 등록 성공
→ PhotoID로 PhotoDefinition 조회
→ CaptureSound 로드
→ PlaySound2D
```

촬영 조건에 실패하거나 이미 촬영한 `PhotoID`이면 사진 전용 사운드를 재생하지 않는다.

### 담당 블루프린트

```text
BP_OrbitViewCharacter
BP_OrbitViewGameMode
카메라 관련 입력 에셋
카메라 HUD
```

### 완료 조건

- 기존 카메라 조작 유지
- `PhotoID` 기준 중복 촬영 차단
- 상태 변경 후 다른 `PhotoID`가 설정되면 재촬영 가능
- 카메라와 3인칭의 `? / ✓` 표시 일치
- 촬영 성공 시 사진별 전용 사운드 재생
- 촬영 기록이 Subsystem에 등록됨

## 5. 공통 API 계약

세 명이 동시에 작업하기 전에 담당자 1이 다음 타입과 함수 원형을 먼저 확정하고, 컴파일 가능한 헤더를 공유한다.

### 증거 정의 및 상태 조회

```cpp
bool GetEvidenceDefinition(
    FName ObjectID,
    FEvidenceDefinition& OutDefinition
) const;

bool GetEvidenceStateDefinition(
    FName StateID,
    FEvidenceStateDefinition& OutState
) const;
```

### 액터 등록

```cpp
bool RegisterEvidenceActor(
    FGuid EvidenceInstanceID,
    FName ObjectID,
    FName& OutCurrentStateID
);
```

### 상호작용

```cpp
bool BeginEvidenceInteraction(
    FGuid EvidenceInstanceID,
    FEvidenceInteractionViewData& OutViewData
);

bool CompleteEvidenceInteraction(
    FGuid EvidenceInstanceID,
    FName ExpectedStateID
);
```

`ExpectedStateID`는 키워드 창이 열려 있는 동안 상태가 변경된 경우 오래된 UI 콜백이 잘못된 상태를 변경하는 것을 방지한다.

### 키워드

```cpp
bool AcquireWord(
    FName WordID,
    EWordAcquisitionSource SourceType,
    FName SourceID
);
```

### 사진

```cpp
bool HasCapturedPhoto(FName PhotoID) const;

bool RegisterCapturedPhoto(
    const FCapturedPhotoRecord& Record
);
```

### 문장

```cpp
bool ValidateSentence(
    FName SentenceID,
    const FSentenceSubmission& Submission,
    FName& OutResultTextID
);
```

## 6. 작업 진행 순서

### 1단계: 공통 기반 작업

담당자 1이 다음을 먼저 완료한다.

- enum
- 정적 정의 구조체
- 런타임 구조체
- Subsystem 헤더 및 공통 API
- 비어 있어도 컴파일 가능한 Subsystem 구현
- `GameplayTags` 모듈 의존성

담당자 2와 3은 공통 헤더가 확정될 때까지 기존 담당 파일 분석과 변경 계획을 준비한다.

### 2단계: 병렬 작업

공통 기반 코드가 통합된 뒤 다음 작업을 동시에 진행한다.

```text
담당자 1
→ Subsystem 내부 구현과 자동 테스트

담당자 2
→ EvidenceActor, Inspection, F 상호작용

담당자 3
→ 카메라, 사진 등록, HUD, 촬영 사운드
```

### 3단계: 데이터와 블루프린트 이전

```text
1. 담당자 1이 Row Struct와 빈 DataTable 생성
2. 담당자 2가 Evidence/State 데이터 입력
3. 담당자 3이 필요한 PhotoID와 CaptureSound 목록 전달
4. 담당자 2가 합의된 데이터를 DataTable에 입력
5. 테스트 Evidence Blueprint 하나만 먼저 전환
6. 검증 후 나머지 Evidence Blueprint 전환
```

### 4단계: 통합

추천 통합 순서는 다음과 같다.

```text
1. 담당자 1의 데이터 구조와 Subsystem
2. 담당자 2의 EvidenceActor와 상호작용
3. 담당자 3의 카메라와 사진
4. DataTable과 블루프린트 데이터
5. 테스트 맵 통합 테스트
6. 기존 필드 제거
```

### 5단계: SaveGame

현재 세 담당자의 통합 작업이 안정된 뒤 별도 작업으로 진행한다. Subsystem 런타임 구조가 확정된 후 SaveGame을 연결해야 중복 수정이 적다.

## 7. 파일 충돌 방지 규칙

| 파일 또는 영역 | 단독 담당자 |
|---|---|
| `Investigation/*` | 담당자 1 |
| `Balhwajeom.Build.cs` | 담당자 1 |
| `BalhwajeomEvidenceActor.*` | 담당자 2 |
| `Interaction/*` | 담당자 2 |
| `BP_Evidence_*` | 담당자 2 |
| `WBP_ObjectLabel` | 담당자 2 |
| `WBP_InspectionMessage` | 담당자 2 |
| `BalhwajeomCameraTargetInterface.*` | 담당자 3 |
| `BalhwajeomEvidenceTypes.*` | 담당자 3 |
| `BalhwajeomPhotoCameraComponent.*` | 담당자 3 |
| `BalhwajeomEvidenceCameraHUD.*` | 담당자 3 |
| `BalhwajeomCameraCharacter.*` | 담당자 3 |
| 카메라 블루프린트와 입력 에셋 | 담당자 3 |

`.uasset`과 `.umap`은 Git에서 병합하기 어렵기 때문에 동일한 블루프린트, DataTable 또는 맵을 두 명이 동시에 수정하지 않는다.

## 8. DataTable 에셋 담당 규칙

DataTable도 `.uasset`이므로 한 명만 직접 수정한다.

추천 역할은 다음과 같다.

```text
담당자 1
→ Row Struct 및 빈 DataTable 생성

담당자 2
→ 실제 Evidence/State/Word/Photo 데이터 입력

담당자 3
→ 필요한 PhotoID와 CaptureSound 목록을 담당자 2에게 전달
```

담당자 3은 DataTable 에셋을 직접 수정하지 않고 필요한 행 정보를 문서나 메시지로 전달한다.

## 9. 통합 테스트 목록

### 데이터 및 Subsystem

- 모든 ID로 올바른 DataTable 행을 조회할 수 있는가
- 잘못된 ID에서 안전하게 실패하는가
- 동일 `WordID`와 `PhotoID`의 중복 획득이 차단되는가

### Evidence 및 상호작용

- 액터 등록 시 `InitialStateID`가 적용되는가
- 거리별 문구가 현재 상태에 따라 변경되는가
- `Once`, `Repeatable`, `ChangeState`가 정의대로 작동하는가
- 상태 변경 이벤트로 메시와 애니메이션이 갱신되는가

### 카메라 및 사진

- 기존 이동, 회전, 줌 기능이 유지되는가
- 초점, 중앙, 화면 포함 비율 판정이 유지되는가
- 신규 `PhotoID`만 등록되는가
- 상태가 변경되어 다른 `PhotoID`가 되면 재촬영할 수 있는가
- 카메라와 3인칭의 `? / ✓` 표시가 일치하는가
- 신규 사진 촬영 성공 시에만 전용 사운드가 재생되는가

## 10. 최종 분배 요약

| 담당자 | 핵심 역할 | 예상 비중 |
|---|---|---:|
| 담당자 1 | 데이터 구조, Settings/DataTable, Subsystem, 자동 테스트 | 35% |
| 담당자 2 | EvidenceActor, 거리, F 상호작용, 증거 블루프린트 | 35% |
| 담당자 3 | 카메라, 촬영, HUD, 사진별 사운드 | 30% |

## 11. 완료 기준

세 담당자의 작업이 통합된 뒤 다음 조건을 모두 만족해야 한다.

- 기존 카메라 조작과 초점 연출이 유지된다.
- 증거 오브젝트가 `ObjectID`와 `CurrentStateID`를 통해 DataTable을 사용한다.
- 거리별 문구가 현재 상태 데이터에서 표시된다.
- F 상호작용의 횟수 제한과 상태 변경이 작동한다.
- 키워드 획득이 공통 Subsystem을 통해 처리된다.
- 촬영 결과가 `PhotoID` 기준으로 Subsystem에 등록된다.
- 카메라, 3인칭, 태블릿이 동일한 촬영 상태를 사용한다.
- 사진별 `CaptureSound`가 신규 촬영 성공 시 재생된다.
- 모든 증거 블루프린트의 데이터 이전이 끝난 뒤 기존 직접 입력 필드를 제거한다.
