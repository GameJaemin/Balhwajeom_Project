# 증거 조사 시스템 구현 인수인계

## 1. 문서 목적

이 문서는 증거 조사 시스템의 현재 설계 결정, 구현 완료 범위, 공개 API, DataTable 구성, 검증 및 테스트 결과와 다음 작업을 정리한다.

프로젝트 경로:

```text
C:\Users\User\Documents\GitHub\Balhwajeom_Project
```

## 2. 확정된 설계 원칙

- `PrimaryDataAsset`과 별도 Database 에셋은 사용하지 않는다.
- 정적 기획 데이터는 7개의 `UDataTable`로 관리한다.
- DataTable 연결은 `UBalhwajeomInvestigationSettings : UDeveloperSettings`가 담당한다.
- 시나리오와 기획자가 Excel/CSV를 자주 사용하므로 `FEvidenceStateDefinition`은 중첩 구조가 아닌 평면 구조다.
- 각 DataTable의 `Row Name`과 행 내부 ID는 반드시 동일하게 유지한다.
- DataTable은 실행 중 변경되지 않는 정의 데이터다.
- 플레이 중 변경되는 상태는 `UBalhwajeomInvestigationSubsystem`이 관리한다.
- 키워드는 증거 상호작용, 브라우저, 메신저 또는 기본 제공으로만 획득한다. 사진에서 키워드를 획득하지 않는다.
- 사진은 `PhotoID`를 기준으로 중복 촬영을 차단한다.
- 오브젝트 상태가 바뀌어 다른 `PhotoID`가 지정되면 별개의 사진으로 다시 촬영할 수 있다.
- 사진별 `CaptureSound`는 신규 사진 등록에 성공했을 때만 재생하는 데이터다. 실제 사운드 재생은 카메라 담당 코드에서 연결한다.
- 현재 단계에는 SaveGame 영속 저장이 포함되지 않는다.

## 3. 구현된 파일

### 정적 정의 및 런타임 타입

```text
Source/Balhwajeom/Public/Investigation/
├─ InvestigationEnums.h
├─ EvidenceDefinitions.h
├─ WordDefinitions.h
├─ PhotoDefinitions.h
├─ SentenceDefinitions.h
└─ InvestigationRuntimeTypes.h
```

### Settings와 Subsystem

```text
Source/Balhwajeom/Public/Investigation/
├─ BalhwajeomInvestigationSettings.h
└─ BalhwajeomInvestigationSubsystem.h

Source/Balhwajeom/Private/Investigation/
├─ BalhwajeomInvestigationSettings.cpp
└─ BalhwajeomInvestigationSubsystem.cpp
```

### 자동 테스트 및 에셋 생성 도구

```text
Source/Balhwajeom/Private/Investigation/Test/
└─ BalhwajeomInvestigationSubsystemTest.cpp

Scripts/Investigation/
└─ create_investigation_data_tables.py
```

추가로 `Source/Balhwajeom/Balhwajeom.Build.cs`에 `DeveloperSettings` 의존성이 들어갔다. `GameplayTags` 의존성도 사용 중이다.

## 4. enum

`InvestigationEnums.h`에 다음 enum이 정의되어 있다.

```text
EEvidenceInteractionBehavior
├─ None
├─ Once
├─ Repeatable
└─ ChangeState

EEvidenceInteractionPresentation
├─ None
├─ SimpleText
└─ KeywordSelectionWindow

EPhotoDescriptionSource
├─ None
├─ ObservationText
├─ InteractionText
└─ Custom

EWordCategory
├─ Object
├─ State
├─ Target
├─ Action
├─ Place
├─ Time
└─ Etc

ESentenceType
├─ PhotoAnalysis
└─ Statement

EOutputTextType
├─ Answer
├─ Statement
├─ Dialogue
└─ Etc

EWordAcquisitionSource
├─ Default
├─ EvidenceInteraction
├─ Browser
└─ Messenger
```

## 5. DataTable 행 구조

### FEvidenceDefinition

증거 오브젝트 종류의 정체성과 최초 상태를 정의한다.

```text
ObjectID
ObjectName
Description
InitialStateID
ClassificationTags
```

### FEvidenceStateDefinition

Excel/CSV 작업을 위해 모든 열이 평면으로 구성되어 있다.

```text
StateID
ObjectID
StateName

InteractionBehavior
InteractionPresentation
NextStateID
InteractionText
KeywordDocumentID

FarLabel
MidLabel
ObservationText

bCanCapture
PhotoID
PreferredFocusDistance
FocusDistanceTolerance
bScaleFocusDistanceWithZoom
```

### 나머지 행 구조

```text
FWordDefinition
FPhotoDefinition
FKeywordDocumentDefinition
FSentenceDefinition
FOutputTextDefinition
```

`FKeywordChoiceDefinition`, `FSentenceWordSlot`, `FSentencePhotoSlot`은 각 행 안에서 사용하는 배열 원소 구조체다.

## 6. 실제 DataTable 에셋

경로:

```text
/Game/Balhwajeom/Data/Investigation
```

생성된 빈 테이블:

| DataTable | Row Structure |
|---|---|
| `DT_EvidenceDefinitions` | `FEvidenceDefinition` |
| `DT_EvidenceStates` | `FEvidenceStateDefinition` |
| `DT_Words` | `FWordDefinition` |
| `DT_Photos` | `FPhotoDefinition` |
| `DT_KeywordDocuments` | `FKeywordDocumentDefinition` |
| `DT_Sentences` | `FSentenceDefinition` |
| `DT_OutputTexts` | `FOutputTextDefinition` |

`Config/DefaultGame.ini`의 다음 섹션에 모두 연결되어 있다.

```ini
[/Script/Balhwajeom.BalhwajeomInvestigationSettings]
```

에디터에서는 다음 위치에서 확인한다.

```text
Project Settings
→ Game
→ Investigation
```

## 7. Subsystem 공개 API 계약

공개 API는 `Three-Person-Implementation-Plan.md`의 계약에 맞춰 구현되어 있다.

```cpp
bool GetEvidenceDefinition(
    FName ObjectID,
    FEvidenceDefinition& OutDefinition
) const;

bool GetEvidenceStateDefinition(
    FName StateID,
    FEvidenceStateDefinition& OutState
) const;

bool RegisterEvidenceActor(
    FGuid EvidenceInstanceID,
    FName ObjectID,
    FName& OutCurrentStateID
);

bool BeginEvidenceInteraction(
    FGuid EvidenceInstanceID,
    FEvidenceInteractionViewData& OutViewData
);

bool CompleteEvidenceInteraction(
    FGuid EvidenceInstanceID,
    FName ExpectedStateID
);

bool AcquireWord(
    FName WordID,
    EWordAcquisitionSource SourceType,
    FName SourceID
);

bool HasCapturedPhoto(FName PhotoID) const;

bool RegisterCapturedPhoto(
    const FCapturedPhotoRecord& Record
);

bool ValidateSentence(
    FName SentenceID,
    const FSentenceSubmission& Submission,
    FName& OutResultTextID
);
```

`ExpectedStateID`는 상호작용 UI가 열린 뒤 증거 상태가 바뀐 경우 오래된 UI 콜백이 현재 상태를 잘못 완료하거나 변경하는 것을 막는다.

## 8. Subsystem 런타임 처리

Subsystem은 다음 데이터를 메모리에서 관리한다.

```text
EvidenceRuntimeStates
AcquiredWords
CapturedPhotos
KeywordDocumentStates
SentenceProgress
```

현재 구현된 동작:

- `ObjectID`를 조회하고 `InitialStateID`가 같은 오브젝트에 속하는지 확인한 뒤 증거 인스턴스를 등록한다.
- 같은 `EvidenceInstanceID`가 같은 `ObjectID`로 다시 등록되면 기존 현재 상태를 반환한다.
- 같은 인스턴스 ID를 다른 `ObjectID`로 등록하는 것은 거부한다.
- `Once`는 해당 `StateID`의 첫 완료만 허용한다.
- `Repeatable`은 현재 상태를 유지하며 반복 완료를 허용한다.
- `ChangeState`는 `NextStateID`가 존재하고 같은 `ObjectID`에 속할 때만 상태를 변경한다.
- 키워드는 `WordID` 기준으로 중복 획득을 거부한다.
- `bUnlockedByDefault` 키워드는 Subsystem 초기화 시 획득 상태로 등록한다.
- 사진은 `PhotoID` 기준으로 중복 등록을 거부한다.
- 촬영 기록의 인스턴스, 오브젝트, 촬영 상태와 상태의 `PhotoID`가 일치해야 등록된다.
- 문장 판정에서는 이미 획득한 키워드와 촬영한 사진만 정답 재료로 인정한다.
- 오답 제출도 현재 문장 슬롯 진행 상태에 보관하고, 최초 정답일 때만 해결 이벤트를 발생시킨다.

## 9. 이벤트

```text
OnEvidenceStateChanged
→ EvidenceInstanceID, PreviousStateID, NewStateID

OnWordAcquired
→ FAcquiredWordRecord

OnPhotoCaptured
→ FCapturedPhotoRecord

OnSentenceSolved
→ SentenceID, ResultTextID
```

EvidenceActor, 카메라, 태블릿 UI는 필요한 이벤트를 구독해 화면과 블루프린트 상태를 갱신한다.

## 10. ID 조회와 검증

Subsystem 초기화 흐름:

```text
InvestigationSettings 읽기
→ DataTable 7개 동기 로드
→ 교차 데이터 유효성 검사
→ 기본 제공 키워드 등록
```

조회 관계:

```text
ObjectID          → DT_EvidenceDefinitions
StateID           → DT_EvidenceStates
WordID            → DT_Words
PhotoID           → DT_Photos
KeywordDocumentID → DT_KeywordDocuments
SentenceID        → DT_Sentences
TextID            → DT_OutputTexts
```

자동 검증 항목:

- 설정된 테이블의 Row Structure
- Row Name과 내부 ID 일치
- `InitialStateID`
- 상태의 `ObjectID`
- `ChangeState.NextStateID`와 오브젝트 소속
- `KeywordDocumentID`
- 촬영 가능한 상태의 `PhotoID`
- `PhotoSentenceID`
- 키워드 선택지의 `ChoiceID` 중복과 `GrantedWordID`
- 문장 키워드/사진 슬롯 인덱스 범위와 중복
- `CorrectWordID`, `CorrectPhotoID`
- `RequiredPhotoCount`
- `ResultTextID`

## 11. 자동 테스트

테스트 파일:

```text
Source/Balhwajeom/Private/Investigation/Test/
└─ BalhwajeomInvestigationSubsystemTest.cpp
```

구현된 테스트 8개:

```text
Balhwajeom.Investigation.SettingsConfiguration
Balhwajeom.Investigation.DefinitionLookup
Balhwajeom.Investigation.DataValidation
Balhwajeom.Investigation.OnceInteraction
Balhwajeom.Investigation.DuplicateWord
Balhwajeom.Investigation.DuplicatePhoto
Balhwajeom.Investigation.InvalidStateTransition
Balhwajeom.Investigation.SentenceValidation
```

마지막 확인 결과:

```text
BalhwajeomEditor Development 빌드 성공
Investigation 자동 테스트 8개 통과
DataTable 생성 스크립트 재실행 성공
```

자동 테스트 실행 예시:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'C:\Users\User\Documents\GitHub\Balhwajeom_Project\Balhwajeom.uproject' `
  -ExecCmds='Automation RunTests Balhwajeom.Investigation;Quit' `
  -TestExit='Automation Test Queue Empty' `
  -unattended -nop4 -nosplash -NullRHI
```

## 12. 아직 하지 않은 작업

다음 작업은 공통 기반 이후의 통합 단계다.

### 담당자 2: EvidenceActor와 3인칭 상호작용 연결

- `ABalhwajeomEvidenceActor`에 `EvidenceInstanceID`, `ObjectID`, `CurrentStateID` 연결
- `BeginPlay`에서 `RegisterEvidenceActor` 호출
- 현재 상태의 거리 표시 문자열을 `InspectionComponent`와 위젯에 적용
- F 입력 시 `BeginEvidenceInteraction` 호출
- UI 처리 후 `CompleteEvidenceInteraction(ExpectedStateID)` 호출
- `OnEvidenceStateChanged`를 받아 메시, 머티리얼, 애니메이션과 충돌 갱신

### 담당자 3: 카메라와 사진 연결

- 기존 자체 수집 배열을 Subsystem 기반으로 전환
- 현재 상태의 `bCanCapture`, `PhotoID`, 초점 거리 사용
- `HasCapturedPhoto(PhotoID)`로 `? / ✓` 표시 결정
- 촬영 성공 시 `FCapturedPhotoRecord`를 만들고 `RegisterCapturedPhoto` 호출
- 신규 등록 성공 시 `FPhotoDefinition.CaptureSound` 재생
- 실제 촬영 이미지 경로를 `ImageRelativePath`에 기록

### 이후 공통 작업

- 실제 시나리오 데이터를 Excel/CSV에서 7개 DataTable로 입력
- 태블릿 메모장·갤러리·진술서 UI 연결
- 키워드/사진 목록 조회 API가 필요해질 때 공개 계약 확장
- KeywordDocument의 선택 진행 상태가 실제로 필요하면 선택 API 추가
- SaveGame 구조와 Subsystem 내보내기/복원 API 구현
- 기존 `BalhwajeomEvidenceTypes` 및 카메라 자체 수집 데이터 제거는 통합 완료 후 진행

## 13. 작업 시 주의사항

- `/Game/Balhwajeom/Maps/Test`와 개발자 테스트 콘텐츠는 Git 제외 정책을 확인한다.
- 새로 만든 `/Game/Balhwajeom/Data/Investigation` DataTable은 공통 기획 데이터이므로 커밋 대상이다.
- 같은 `.uasset` 또는 `.umap`을 여러 담당자가 동시에 수정하지 않는다.
- 현재 작업 트리에 다른 사용자의 변경이 있을 수 있으므로 `git reset --hard`, 강제 체크아웃 등으로 되돌리지 않는다.
- UE 빌드 로그의 `bAllowUBALocalExecutor` deprecated 경고는 프로젝트의 기존 빌드 설정 경고이며 Investigation 구현 실패가 아니다.
