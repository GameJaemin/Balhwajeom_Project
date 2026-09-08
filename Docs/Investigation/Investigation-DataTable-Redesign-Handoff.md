# 증거 조사 시스템 — DataTable/Subsystem 재설계 인수인계

## 0. 이 문서 목적과 사용법

이 문서는 《발화점: 세 자백》증거 조사 시스템(`UBalhwajeomInvestigationSubsystem` 및 관련 7개 DataTable)의 스키마를 최신 기획 변경사항에 맞춰 재검토하고 합의한 내용을 정리한다. 최초 구현은 [Investigation-System-Implementation-Handoff.md](./Investigation-System-Implementation-Handoff.md), [Three-Person-Implementation-Plan.md](./Three-Person-Implementation-Plan.md)를 참고하되, **이 문서가 그 이후의 최신 결정 사항을 덮어쓴다.**

섹션 구성:

- **1절**: 이 시스템 전반에 적용되는 설계 원칙 (앞으로 스키마를 바꿀 때도 이 기준으로 판단)
- **2~3절**: 지금 실제 코드에 있는 그대로의 상태 (사실 그대로, 추측 없음)
- **4절**: 논의를 통해 결정됐지만 아직 코드에 반영 안 된 것 — **최우선 작업 대상**
- **5절**: 아직 결론이 안 나서 임의로 정하면 안 되는 것
- **6절**: 우선순위 체크리스트

AI가 이 문서만 보고 작업을 이어가야 한다면, 4절부터 순서대로 구현하고 5절 항목은 반드시 사용자에게 먼저 확인할 것.

## 1. 핵심 설계 원칙

1. **DataTable = 플레이 중 절대 안 바뀌는 정적 정의 데이터.** CSV Import/Reimport로 기획자가 직접 관리한다. 예: 문장 템플릿, 상태별 촬영 가능 여부, 초점 거리 기본값.
2. **DeveloperSettings(`UBalhwajeomInvestigationSettings`) = 프로젝트 전역에 하나만 있으면 되는 설정.** DataTable "행"이 아니라 "어떤 테이블을 쓸지" 같은 단일 값들.
3. **Subsystem 런타임 상태(`TMap`/`TSet` 멤버) = 플레이 중 실제로 바뀌는 값.** 누가 뭘 획득했는지, 지금 어느 상태인지, 이미 풀었는지 등. **DataTable에는 절대 넣지 않는다.** (예: `bCanCapture`는 "이 상태가 촬영 가능한 종류인가"라는 정의값이고, "실제로 촬영했는가"는 `CapturedPhotos` 런타임 맵에만 있음)
4. **Row Name은 항상 그 행의 내부 ID 필드 값과 동일해야 한다.** Subsystem 초기화 시 자동 검증됨(`ValidateTableRowIDs`).
5. **1(부모):N(자식) 관계 + 자식 필드가 3개 이상이면 별도 테이블로 분리한다.** 자식 필드가 1~2개뿐이면 중첩 배열(`((...),(...))` 튜플 CSV 문법)로 충분하다.
   - 분리한 예: `DT_EvidenceDefinitions`↔`DT_EvidenceStates` (오브젝트 1개가 상태 여러 개), `DT_KeywordDocuments`↔`DT_KeywordChoices` (문서 1개가 선택지 여러 개)
   - 중첩으로 충분한 예: `FSentenceDefinition.WordSlots`/`PhotoSlots` (슬롯 하나당 필드 2개뿐)
6. **ID 참조로 정규화한다** (같은 값을 여러 테이블에 중복 저장하지 않음). 언리얼은 CSV의 FName 참조를 자동으로 검증해주지 않으므로, Subsystem 초기화 시 `ValidateLoadedDataTables()`가 모든 ID 참조를 직접 순회하며 교차검증한다. 이 검증 로직은 새 참조 필드를 추가할 때마다 같이 늘려야 한다.
7. **필드를 추가하기 전에 "① 코드에서 실제로 읽는가 ② 기획 문서에 대응 요구사항이 있는가"를 확인한다.** 둘 다 아니면 만들지 않거나 즉시 삭제한다. 이번 정리에서 `EWordCategory`(enum째로), `FEvidenceDefinition.Description`/`ClassificationTags`, `FPhotoDefinition.PhotoTags`/`CaptureSound`가 이 기준으로 제거됐다.
8. **값이 사실상 항상 동일하다면**(예: 초점 거리 관련 3개 필드), 구조체 필드 자체는 남기고 C++ 기본값을 지정해두되 CSV 컬럼은 생략한다. 언리얼 DataTable CSV import는 헤더에 없는 컬럼은 건드리지 않으므로 모든 행이 자동으로 기본값을 갖는다. 구조체를 삭제하거나 전역 Settings로 옮기지 않는다 — 나중에 예외 케이스가 생기면 그 행만 값을 채우면 되므로 유연성을 유지한다.
9. **키워드 획득은 `AcquireWord(WordID, SourceType, SourceID)` 하나로 통일.** "어디서 획득했는지"는 DataTable에 저장하지 않고, 그 키워드를 지급하는 콘텐츠(사진, F조사 문서, 브라우저, 메신저) 쪽 코드가 호출 시점에 결정한다.

## 2. 파일 구조 (실제 코드)

```
Source/Balhwajeom/Public/Investigation/
├─ InvestigationEnums.h
├─ EvidenceDefinitions.h        (FEvidenceDefinition, FEvidenceStateDefinition)
├─ WordDefinitions.h            (FWordDefinition, FKeywordChoiceDefinition, FKeywordDocumentDefinition)
├─ PhotoDefinitions.h           (FPhotoDefinition)
├─ SentenceDefinitions.h        (FSentenceWordSlot, FSentencePhotoSlot, FSentenceDefinition)
├─ InvestigationRuntimeTypes.h  (런타임 전용 구조체)
├─ BalhwajeomInvestigationSettings.h
└─ BalhwajeomInvestigationSubsystem.h

Source/Balhwajeom/Private/Investigation/
├─ BalhwajeomInvestigationSettings.cpp
├─ BalhwajeomInvestigationSubsystem.cpp
└─ Test/BalhwajeomInvestigationSubsystemTest.cpp

Content/Balhwajeom/Data/Investigation/
├─ DT_EvidenceDefinitions.uasset
├─ DT_EvidenceStates.uasset
├─ DT_Words.uasset
├─ DT_Photos.uasset
├─ DT_KeywordDocuments.uasset
└─ DT_Sentences.uasset
```

## 3. DataTable별 현재 필드 (as-is)

### 3.1 DT_EvidenceDefinitions — `FEvidenceDefinition`

```
ObjectID        FName   행 이름과 동일, 증거 오브젝트 종류 식별자
ObjectName      FText
InitialStateID  FName   → DT_EvidenceStates.StateID. 상태가 하나뿐이고 절대 안 바뀌는 오브젝트도 필수로 채워야 함
                        (CurrentStateID의 최초값을 여기서 가져오지 않으면 아무 것도 동작하지 않음)
```

`Description`, `ClassificationTags`는 코드/기획 어디에도 사용처가 없어 삭제됨.

### 3.2 DT_EvidenceStates — `FEvidenceStateDefinition`

오브젝트 1개가 여러 상태(닫힘→열림 등)를 가질 수 있어 `DT_EvidenceDefinitions`와 분리됨.

```
StateID                      FName   행 이름과 동일
ObjectID                     FName   → DT_EvidenceDefinitions.ObjectID
StateName                    FText   에디터/기획 확인용, 게임 로직 판정에는 사용 안 함
InteractionBehavior          Enum    None / Once / Repeatable / ChangeState
InteractionPresentation      Enum    None / SimpleText / KeywordSelectionWindow
NextStateID                  FName   InteractionBehavior=ChangeState일 때 필수
InteractionText              FText   InteractionPresentation=SimpleText일 때 사용
KeywordDocumentID            FName   InteractionPresentation=KeywordSelectionWindow일 때 필수 → DT_KeywordDocuments.KeywordDocumentID
FarLabel                     FText   ⚠️ 5.1절 참고 — 실제 표시 조건 미확인
MidLabel                     FText   중간 거리 텍스트
ObservationText              FText   근거리 관찰 텍스트. 사진 설명 소스(DescriptionSource=ObservationText)로도 재사용됨
bCanCapture                  bool    "이 상태를 촬영할 수 있는 종류인가" (실제 촬영 여부 아님! 그건 Subsystem.HasCapturedPhoto)
PhotoID                      FName   bCanCapture=true일 때 필수 → DT_Photos.PhotoID
PreferredFocusDistance       float   기본값 700.0, 예외 없으면 CSV 컬럼 자체를 생략
FocusDistanceTolerance       float   기본값 300.0, 예외 없으면 CSV 컬럼 자체를 생략
bScaleFocusDistanceWithZoom  bool    기본값 true, 예외 없으면 CSV 컬럼 자체를 생략
```

### 3.3 DT_Words — `FWordDefinition`

```
WordID              FName   행 이름과 동일
DisplayWord         FText
Description         FText
bUnlockedByDefault  bool    true면 Subsystem 초기화 시 자동 획득 처리
```

`Category`(`EWordCategory` enum)는 코드/기획 어디서도 안 쓰여 필드와 enum 자체를 삭제함. "획득 조건/획득 지점"은 이 테이블이 아니라 **지급하는 콘텐츠 쪽**(`DT_Photos.GrantedWordIDs`, `DT_KeywordChoices.GrantedWordID`)에 작성한다 — DT_Words는 "이 키워드가 뭔지"만 안다.

### 3.4 DT_Photos — `FPhotoDefinition`

```
PhotoID              FName   행 이름과 동일
PhotoName            FText
DescriptionSource    Enum    None / ObservationText / InteractionText / Custom
CustomDescription    FText   DescriptionSource=Custom일 때만 사용
PhotoSentenceID      FName   사진 분석(빈칸 채우기) 문장 ID → DT_Sentences(SentenceType=PhotoAnalysis).
                             비어있으면 "스토리 사진"(별도 PhotoType enum 불필요, 이 필드 유무로 종류 구분)
StatementSentenceID  FName   이 사진이 표시될 인물 폴더 결정용 → DT_Sentences(SentenceType=Statement).SentenceID.
                             "진술서 증거로 채택되는지"와는 별개 개념 — 그건 그 진술서의 PhotoSlots.CorrectPhotoID가 결정
WorldStoryText       FText   촬영 직후 & 폴더에서 재열람 시 월드 고정 3D 텍스트로 표시
StoryVoice           TSoftObjectPtr<USoundBase>  WorldStoryText 낭독 음성
```

`PhotoTags`(사용처 없음), `CaptureSound`(별도 촬영효과음, 내레이션만 쓰기로 결정하며 제외)는 삭제됨. **`GrantedWordIDs`는 아직 미구현 — 4.1절 참고.**

### 3.5 DT_KeywordDocuments — `FKeywordDocumentDefinition` (곧 구조 변경 예정, 4.3절 참고)

```
KeywordDocumentID     FName
DocumentText          FText
KeywordChoices        TArray<FKeywordChoiceDefinition>   ⚠️ 별도 테이블(DT_KeywordChoices)로 분리 예정
bCloseAfterSelection  bool                                ⚠️ 삭제 예정(5.2절 최종 확인만 남음)
```

`FKeywordChoiceDefinition`(현재는 위 배열의 내부 struct, 독립 테이블 아님):

```
ChoiceID       FName
DisplayText    FText
GrantedWordID  FName   → DT_Words.WordID
```

### 3.6 DT_Sentences — `FSentenceDefinition` (곧 필드 추가/삭제 예정, 4.2절 참고)

```
SentenceID          FName   행 이름과 동일
SentenceType        Enum    PhotoAnalysis / Statement
SentenceTemplate    FText   빈칸 문제 문장
WordSlots           TArray<{SlotIndex 0~4, CorrectWordID}>   최대 5칸
PhotoSlots          TArray<{SlotIndex 0~1, CorrectPhotoID}>  최대 2칸
RequiredPhotoCount  int32   정답 판정에 필요한 사진 정답 개수
ResultTextID        FName   ⚠️ 삭제 예정 — 아무도 참조 안 하는 죽은 ID (SentenceID로 이미 조회 가능)
ResultText          FText   정답 처리 후 SentenceTemplate 대신 화면에 보여줄 자연어 완성 문장
DesignerNote        FText
```

`SentenceTemplate` → `ResultText` 전환은 **UI 코드가 판정 결과를 보고 어느 텍스트를 그릴지 분기하는 것**으로 처리한다 (새 데이터 필드 불필요). "이미 풀렸는지"는 `ValidateSentence`의 반환값 또는 4.4절의 `IsSentenceSolved`로 확인.

**`ChapterID`, `CharacterID`, `FolderName`, `FolderSortOrder`, `LieText`는 아직 미구현 — 4.2절 참고.**

### 3.7 Subsystem 공개 API (현재)

```cpp
bool GetEvidenceDefinition(FName ObjectID, FEvidenceDefinition& OutDefinition) const;
bool GetEvidenceStateDefinition(FName StateID, FEvidenceStateDefinition& OutState) const;
bool GetPhotoDefinition(FName PhotoID, FPhotoDefinition& OutDefinition) const;
bool RegisterEvidenceActor(FGuid EvidenceInstanceID, FName ObjectID, FName& OutCurrentStateID);
bool BeginEvidenceInteraction(FGuid EvidenceInstanceID, FEvidenceInteractionViewData& OutViewData);
bool CompleteEvidenceInteraction(FGuid EvidenceInstanceID, FName ExpectedStateID);
bool AcquireWord(FName WordID, EWordAcquisitionSource SourceType, FName SourceID);
bool HasCapturedPhoto(FName PhotoID) const;
bool RegisterCapturedPhoto(const FCapturedPhotoRecord& Record);
bool ValidateSentence(FName SentenceID, const FSentenceSubmission& Submission,
    FName& OutResultTextID, FText& OutResultText);   // ⚠️ OutResultTextID 제거 예정
```

이벤트: `OnEvidenceStateChanged`, `OnWordAcquired`, `OnPhotoCaptured`, `OnSentenceSolved`

`ValidateSentence`의 PhotoSlot 검증은 이미 "촬영 여부"뿐 아니라 "그 사진의 `PhotoSentenceID`가 가리키는 분석 문장이 실제로 풀렸는지"까지 확인하도록 구현되어 있다 (미완성/스토리 사진을 진술서 증거로 못 쓰게 막는 로직).

### 3.8 Subsystem 런타임 상태 (SaveGame 대상, 현재는 메모리에만 존재)

```cpp
TMap<FGuid, FEvidenceRuntimeState>          EvidenceRuntimeStates;
TMap<FName, FAcquiredWordRecord>            AcquiredWords;
TMap<FName, FCapturedPhotoRecord>           CapturedPhotos;
TMap<FName, FKeywordDocumentRuntimeState>   KeywordDocumentStates;
TMap<FName, FSentenceRuntimeProgress>       SentenceProgress;
```

SaveGame 구현 시 이 5개를 그대로 직렬화하면 된다. ID만 저장하고 표시용 텍스트(DisplayWord 등)는 저장하지 않는다 — 로드 시 DataTable에서 다시 조회.

## 4. 결정됐지만 아직 미구현 (우선순위 순)

### 4.1 `FPhotoDefinition.GrantedWordIDs` 추가 — 최우선

기획이 "추리용/스토리 사진 촬영 시 오브젝트에 할당된 키워드를 0~n개 획득한다"를 명시적으로 확정함 (기존 "사진에서 키워드 획득 안 함" 원칙은 폐기됨).

```cpp
// PhotoDefinitions.h, FPhotoDefinition에 추가
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
TArray<FName> GrantedWordIDs;
```

- `InvestigationEnums.h`의 `EWordAcquisitionSource`에 `PhotoCapture` 추가
- `RegisterCapturedPhoto`가 `true`를 반환할 때, `GrantedWordIDs`를 순회하며 각각 `AcquireWord(WordID, EWordAcquisitionSource::PhotoCapture, Record.PhotoID)` 호출
- `ValidateLoadedDataTables()`에 `GrantedWordIDs`의 각 WordID가 `DT_Words`에 존재하는지 교차검증 추가
- CSV 배열 문법: `(WORD_01_001,WORD_01_002)` (구조체 배열이 아닌 단순 FName 배열이라 괄호 한 겹)

### 4.2 `FSentenceDefinition` 필드 정리

**삭제:**
```cpp
FName ResultTextID;  // 삭제
```
- `ValidateLoadedDataTables()`의 `Sentence->ResultTextID.IsNone()` 체크 삭제
- `ValidateSentence` 시그니처 변경:
  ```cpp
  // 변경 전
  bool ValidateSentence(FName SentenceID, const FSentenceSubmission& Submission,
      FName& OutResultTextID, FText& OutResultText);
  // 변경 후
  bool ValidateSentence(FName SentenceID, const FSentenceSubmission& Submission,
      FText& OutResultText);
  ```
- `OnSentenceSolved` 델리게이트가 `ResultTextID`를 파라미터로 넘기고 있다면 같이 정리 (현재 `FOnSentenceSolved(FName SentenceID, FName ResultTextID)` → `ResultTextID` 제거 여부 확인)

**추가:**
```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
FName ChapterID = NAME_None;        // 챕터 구분용

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
FName CharacterID = NAME_None;      // 이 진술서를 쓴 인물 (Statement 타입에서 사용)

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
FText FolderName;                   // 태블릿에 표시할 폴더 이름

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
int32 FolderSortOrder = 0;          // 폴더 정렬 순서

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (MultiLine = "true"))
FText LieText;                      // 인물이 한 거짓말 (빈칸 없는 고정 문장, SentenceTemplate과 별개)
                                     // Statement 타입에만 채움, PhotoAnalysis 타입은 비워둠
```

전제 조건(사용자 확인 완료): **폴더가 있는 인물은 항상 진술서(Statement 타입 문장)도 있다.** 그래서 `DT_Photos.StatementSentenceID` → 이 `CharacterID`/`FolderName` 경유 방식이 성립하며, 별도의 `DT_Characters` 테이블은 불필요하다.

### 4.3 KeywordChoices를 별도 테이블로 분리

`FKeywordChoiceDefinition`을 내부 struct에서 독립 `FTableRowBase`로 변경:

```cpp
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FKeywordChoiceDefinition : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
    FName ChoiceID = NAME_None;   // 행 이름과 동일. 문서 전체에서 전역 유일해야 함

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
    FName KeywordDocumentID = NAME_None;   // → DT_KeywordDocuments.KeywordDocumentID

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
    FText DisplayText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
    FName GrantedWordID = NAME_None;   // → DT_Words.WordID

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
    int32 SortOrder = 0;   // 같은 문서 안에서 표시 순서
};
```

`FKeywordDocumentDefinition`은 `KeywordChoices` 배열과 `bCloseAfterSelection`을 제거하고 `KeywordDocumentID`, `DocumentText`만 남긴다 (5.2절 최종 확인 후).

추가 작업:
- `UBalhwajeomInvestigationSettings`에 `KeywordChoicesTable` (`TSoftObjectPtr<UDataTable>`) 추가
- `BalhwajeomInvestigationSubsystem`에 `KeywordChoicesTable` 멤버 + 로드 로직 추가
- 신규 공개 API:
  ```cpp
  bool GetKeywordDocumentDefinition(FName KeywordDocumentID, FKeywordDocumentDefinition& OutDefinition) const;
  void GetKeywordChoicesForDocument(FName KeywordDocumentID, TArray<FKeywordChoiceDefinition>& OutChoices) const; // SortOrder 정렬
  ```
- 검증 로직 재작성: 문서 내부 배열을 순회하던 기존 코드를, `KeywordChoicesTable` 전체를 순회하며 `KeywordDocumentID`별로 그룹핑해서 ChoiceID 중복/GrantedWordID 존재/KeywordDocumentID 참조 유효성 확인하는 방식으로 변경. 같은 문서 내 `SortOrder` 중복도 검사 권장(필수는 아님).

### 4.4 `IsSentenceSolved` 공개 API 추가

```cpp
UFUNCTION(BlueprintPure, Category = "Investigation|Sentences")
bool IsSentenceSolved(FName SentenceID) const;
// 구현: const FSentenceRuntimeProgress* P = SentenceProgress.Find(SentenceID); return P != nullptr && P->bSolved;
```

용도: 태블릿 폴더의 파일 아이콘에 `?`/`✓` 표시. `ValidateSentence`(판정)와 달리 순수 조회용이며, 판정 로직 안의 중복 이벤트 방지(`bWasAlreadySolved`)와는 별개 용도.

### 4.5 인물별 목록 조회 API (우선순위 낮음)

```cpp
void GetStatementSentencesForCharacter(FName CharacterID, TArray<FSentenceDefinition>& OutSentences) const;
void GetPhotosForCharacter(FName CharacterID, TArray<FPhotoDefinition>& OutPhotos) const;
```

- 태블릿에서 인물 폴더를 열 때 그 안에 뭘 나열할지 결정하는 용도
- `GetPhotosForCharacter`는 각 사진의 `StatementSentenceID`로 문장을 찾고, 그 문장의 `CharacterID`가 일치하는지로 판단 (사진에 `CharacterID`를 직접 중복 저장하지 않음)
- 소규모 프로토타입 단계에서는 레벨 블루프린트에 표시할 ID 목록을 하드코딩해서 임시로 우회 가능 — 실제로 필요해질 때 구현해도 무방

## 5. 아직 결론 안 남 (임의로 정하지 말 것)

### 5.1 `FarLabel`의 실제 표시 조건

기획서: "가장 먼 거리에서는 거리별 텍스트를 표시하지 않는다"고 명시. 그런데 `FarLabel` 필드가 실제로 화면에 노출되는 시점이 불분명함 — `InspectionComponent.MaxDisplayDistance` 경계와 관련해 4단계 구간(초원거리=텍스트없음 / 먼거리=FarLabel / 중간거리=MidLabel / 근거리=ObservationText)이 실제로 존재하는지, 아니면 `FarLabel`이 죽은 필드인지 기획 확인 필요. **일단 필드는 삭제하지 않고 유지하기로 함.**

### 5.2 `bCloseAfterSelection` 삭제 최종 확인

여러 차례 새 CSV 스펙에서 계속 빠져있어 삭제로 거의 확정된 상태지만, 명시적인 "삭제해" 지시는 아직 못 받음. 4.3절 작업(KeywordChoices 테이블 분리) 진행 전에 최종 확인 권장.

## 6. 우선순위 체크리스트

- [ ] 4.1 `Photo.GrantedWordIDs` + `EWordAcquisitionSource::PhotoCapture` (핵심 게임플레이 규칙, 최우선)
- [ ] 4.2 `Sentence.ResultTextID` 삭제 + `ValidateSentence` 시그니처 변경
- [ ] 4.2 `Sentence.ChapterID`/`CharacterID`/`FolderName`/`FolderSortOrder`/`LieText` 추가
- [ ] 5.2 확인 후 → 4.3 KeywordChoices 별도 테이블 분리 (구조체·Settings·Subsystem 조회/검증·신규 공개 API)
- [ ] 4.4 `IsSentenceSolved` 공개 API
- [ ] 4.5 인물별 목록 조회 API (필요해지면)
- [ ] 5.1 기획 확인 후 `FarLabel` 처리 방향 결정
- [ ] 위 변경사항을 반영해 `Scripts/Investigation/*.csv` 예시 파일 갱신
- [ ] `BalhwajeomInvestigationSubsystemTest.cpp`에 변경된 필드/시그니처에 맞는 테스트 갱신
- [ ] 담당자2(EvidenceActor·상호작용)·담당자3(카메라·사진) 통합은 이 데이터 작업이 끝난 뒤 진행 (아직 미착수 상태, [Investigation-System-Implementation-Handoff.md](./Investigation-System-Implementation-Handoff.md) 12절 참고)
