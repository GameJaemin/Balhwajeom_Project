# 증거 조사 시스템 — DataTable/Subsystem 재설계 인수인계

## 0. 이 문서 목적과 사용법

이 문서는 《발화점: 세 자백》증거 조사 시스템(`UBalhwajeomInvestigationSubsystem` 및 관련 8개 DataTable)의 스키마를 최신 기획 변경사항에 맞춰 재검토하고 합의한 내용을 정리한다. 최초 구현은 [Investigation-System-Implementation-Handoff.md](./Investigation-System-Implementation-Handoff.md), [Three-Person-Implementation-Plan.md](./Three-Person-Implementation-Plan.md)를 참고하되, **이 문서가 그 이후의 최신 결정 사항을 덮어쓴다.**

섹션 구성:

- **1절**: 이 시스템 전반에 적용되는 설계 원칙
- **2~3절**: 지금 실제 코드에 있는 그대로의 상태 (사실 그대로, 추측 없음)
- **4절**: 논의를 통해 결정됐지만 아직 코드에 반영 안 된 것 — **최우선 작업 대상**
- **5절**: 아직 결론이 안 나서 임의로 정하면 안 되는 것
- **6절**: 우선순위 체크리스트

AI가 이 문서만 보고 작업을 이어가야 한다면, 4절부터 순서대로 구현하고 5절 항목은 반드시 사용자에게 먼저 확인할 것.

## 1. 핵심 설계 원칙

1. **DataTable = 플레이 중 절대 안 바뀌는 정적 정의 데이터.** CSV Import/Reimport로 기획자가 직접 관리한다.
2. **DeveloperSettings(`UBalhwajeomInvestigationSettings`) = 프로젝트 전역에 하나만 있으면 되는 설정.** "어떤 테이블을 쓸지" 같은 단일 값들.
3. **Subsystem 런타임 상태(`TMap`/`TSet` 멤버) = 플레이 중 실제로 바뀌는 값.** DataTable에는 절대 넣지 않는다. (예: `bCanCapture`는 "촬영 가능한 종류인가"라는 정의값이고, "실제로 촬영했는가"는 `CapturedPhotos` 런타임 맵에만 있음)
4. **Row Name은 항상 그 행의 내부 ID 필드 값과 동일해야 한다.** Subsystem 초기화 시 자동 검증됨.
5. **1(부모):N(자식) 관계 + 자식 필드가 3개 이상이면 별도 테이블로 분리한다.** 자식 필드가 1~2개뿐이면 중첩 배열(`((...),(...))` 튜플 CSV 문법)로 충분하다.
   - 분리한 예: `DT_EvidenceDefinitions`↔`DT_EvidenceStates`, `DT_Sentences`(진술서 문장)↔`DT_Characters`(인물/폴더)
   - 중첩으로 충분한 예: `FSentenceDefinition.WordSlots`/`PhotoSlots`
6. **ID 참조로 정규화하되, 참조 단계(indirection)가 실제로 혼란을 일으키면 직접 참조로 되돌린다.** 정규화는 "같은 값을 중복 저장하지 않는 것"이 목적이지 그 자체가 목적이 아니다. 조회를 위해 테이블을 두 단계 이상 거쳐야 하고 그게 반복적으로 이해를 어렵게 만든다면, 약간의 중복을 감수하고 직접 참조로 단순화하는 게 낫다. (예: `DT_Photos`가 처음엔 `StatementSentenceID`로 진술서 문장을 거쳐 인물을 찾는 구조였으나, 계속 혼란을 일으켜 `CharacterID` 직접 참조로 변경함)
7. **언리얼은 CSV의 FName 참조를 자동으로 검증해주지 않으므로, Subsystem 초기화 시 모든 ID 참조를 직접 순회하며 교차검증한다** (`ValidateLoadedDataTables()`). 새 참조 필드를 추가할 때마다 이 검증도 같이 늘린다.
8. **필드를 추가하기 전에 "① 코드에서 실제로 읽는가 ② 기획 문서에 대응 요구사항이 있는가"를 확인한다.** 둘 다 아니면 만들지 않거나 즉시 삭제한다. (`EWordCategory`, `FEvidenceDefinition.Description`/`ClassificationTags`, `FPhotoDefinition.PhotoTags`/`CaptureSound`, `ChapterID` 등이 이 기준으로 도입되지 않았거나 제거됨)
9. **값이 사실상 항상 동일하다면**, 구조체 필드는 남기고 C++ 기본값을 지정해두되 CSV 컬럼은 생략한다 (헤더에 없는 컬럼은 건드리지 않으므로 자동으로 기본값 적용). 구조체를 삭제하거나 전역 Settings로 옮기지 않는다. (예: `PreferredFocusDistance` 등 초점 관련 3개 필드)
10. **"관심사가 다르면 카디널리티가 1:1이어도 분리한다.** `DT_Sentences`(문제 문장 내용)와 `DT_Characters`(인물/폴더 메타데이터)가 우연히 인물당 진술서 1개로 1:1 관계였지만, 서로 다른 관심사라 분리했다.
11. **키워드 획득은 `AcquireWord(WordID, SourceType, SourceID)` 하나로 통일.** "어디서 획득했는지"는 DataTable에 저장하지 않고, 그 키워드를 지급하는 콘텐츠(사진, F조사 문서, 브라우저, 메신저) 쪽 코드가 호출 시점에 결정한다.

## 2. 파일 구조 (실제 코드)

```
Source/Balhwajeom/Public/Investigation/
├─ InvestigationEnums.h
├─ EvidenceDefinitions.h        (FEvidenceDefinition, FEvidenceStateDefinition)
├─ WordDefinitions.h            (FWordDefinition, FKeywordChoiceDefinition, FKeywordDocumentDefinition)
├─ PhotoDefinitions.h           (FPhotoDefinition)
├─ SentenceDefinitions.h        (FSentenceWordSlot, FSentencePhotoSlot, FSentenceDefinition)
├─ CharacterDefinitions.h       (FCharacterDefinition)  ← 신규
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

`DT_Characters.uasset`은 아직 콘텐츠 폴더에 생성 전 (구조체는 코드에 존재, 에셋 신규 생성 필요).

## 3. DataTable별 현재 필드 (as-is, 코드 그대로)

### 3.1 DT_EvidenceDefinitions — `FEvidenceDefinition`

```
ObjectID        FName   행 이름과 동일
ObjectName      FText
InitialStateID  FName   → DT_EvidenceStates.StateID. 상태가 하나뿐인 오브젝트도 필수
```

`Description`, `ClassificationTags`는 사용처 없어 삭제됨.

### 3.2 DT_EvidenceStates — `FEvidenceStateDefinition`

```
StateID                      FName   행 이름과 동일
ObjectID                     FName   → DT_EvidenceDefinitions.ObjectID
StateName                    FText   에디터 확인용, 게임 로직엔 안 쓰임
InteractionBehavior          Enum    None / Once / Repeatable / ChangeState
InteractionPresentation      Enum    None / SimpleText / KeywordSelectionWindow
NextStateID                  FName   ChangeState일 때 필수
InteractionText              FText   SimpleText일 때 사용
KeywordDocumentID            FName   KeywordSelectionWindow일 때 필수 → DT_KeywordDocuments.KeywordDocumentID
FarLabel                     FText   ⚠️ 5.1절 참고 — 실제 표시 조건 미확인, 필드는 유지하기로 함
MidLabel                     FText   중간 거리 텍스트
ObservationText              FText   근거리 관찰 텍스트, 사진 설명 소스로도 재사용
bCanCapture                  bool    "촬영 가능한 종류인가" (실제 촬영 여부 아님)
PhotoID                      FName   bCanCapture=true일 때 필수 → DT_Photos.PhotoID
PreferredFocusDistance       float   기본값 700.0, 예외 없으면 CSV 컬럼 생략
FocusDistanceTolerance       float   기본값 300.0, 예외 없으면 CSV 컬럼 생략
bScaleFocusDistanceWithZoom  bool    기본값 true, 예외 없으면 CSV 컬럼 생략
```

### 3.3 DT_Words — `FWordDefinition`

```
WordID              FName   행 이름과 동일
DisplayWord         FText
Description         FText
bUnlockedByDefault  bool    true면 Subsystem 초기화 시 자동 획득
```

`Category`(`EWordCategory`)는 사용처 없어 필드+enum 자체를 삭제함. "획득 조건/획득 지점"은 지급 콘텐츠 쪽(`DT_Photos.GrantedWordIDs`(미구현, 4.1절), `DT_KeywordChoices.GrantedWordID`)에 작성.

### 3.4 DT_Photos — `FPhotoDefinition`

```
PhotoID              FName   행 이름과 동일
PhotoName            FText
DescriptionSource    Enum    None / ObservationText / InteractionText / Custom
CustomDescription    FText   DescriptionSource=Custom일 때만 사용
PhotoSentenceID      FName   사진 분석(빈칸 채우기) 문장 → DT_Sentences(SentenceType=PhotoAnalysis).
                             비어있으면 "스토리 사진"(별도 PhotoType enum 없이 이 필드 유무로 구분).
                             선택 사항 — 스토리 사진은 비워둠
CharacterID          FName   이 사진이 표시될 인물 폴더 → DT_Characters.CharacterID.
                             **모든 사진(추리용/스토리 불문) 필수** — Subsystem이 빈 값이면 검증 에러로 잡음.
                             "진술서 증거로 채택되는지"와는 완전히 별개 — 그건 그 진술서의
                             PhotoSlots.CorrectPhotoID가 결정. 인물의 문장을 거치지 않고 직접 참조함
                             (한때 StatementSentenceID로 문장을 거쳐 인물을 찾는 구조였으나 혼란을 줘서 직접 참조로 변경)
WorldStoryLines      TArray<FText>  촬영 직후 & 폴더 재열람 시 월드 고정 3D 텍스트. 여러 문장을 배열에 순서대로 담고,
                     표시할 때는 그 순서대로 출력(현재 태블릿 팝업에서는 줄바꿈으로 이어붙여 표시)
StoryVoice           TSoftObjectPtr<USoundBase>  WorldStoryLines 낭독 음성
```

`PhotoTags`(사용처 없음), `CaptureSound`(별도 촬영효과음, 내레이션만 쓰기로 결정)는 삭제됨. `GrantedWordIDs`는 아직 미구현 — 4.1절 참고.

### 3.5 DT_KeywordDocuments — `FKeywordDocumentDefinition` (곧 구조 변경 예정, 4.3절 참고)

```
KeywordDocumentID     FName
DocumentText          FText
KeywordChoices        TArray<FKeywordChoiceDefinition>   ⚠️ 별도 테이블(DT_KeywordChoices)로 분리 예정
bCloseAfterSelection  bool                                ⚠️ 삭제 예정(5.2절 최종 확인만 남음)
```

`FKeywordChoiceDefinition`(현재는 내부 struct, 독립 테이블 아님): `ChoiceID`, `DisplayText`, `GrantedWordID`.

### 3.6 DT_Sentences — `FSentenceDefinition`

```
SentenceID          FName   행 이름과 동일
SentenceType        Enum    PhotoAnalysis / Statement
CharacterID         FName   이 문장(주로 Statement 타입)이 속한 인물 → DT_Characters.CharacterID.
                            PhotoAnalysis 타입은 비워둠
SentenceTemplate    FText   빈칸 문제 문장
WordSlots           TArray<{SlotIndex 0~4, CorrectWordID}>   최대 5칸
bWordOrderMatters   bool    true(기본)=슬롯 위치까지 정확히 일치해야 정답.
                            false=위치 무관, 필요한 단어들을 다중집합으로만 비교
PhotoSlots          TArray<{SlotIndex 0~1, CorrectPhotoID}>  최대 2칸
RequiredPhotoCount  int32   정답 판정에 필요한 사진 정답 개수
ResultTextID        FName   ⚠️ 삭제 예정 — 아무도 참조 안 하는 죽은 ID, 4.2절 참고
ResultText          FText   정답 처리 후 SentenceTemplate 대신 화면에 보여줄 자연어 완성 문장
DesignerNote        FText
```

`ChapterID`는 코드/기획 어디서도 쓰임이 확인되지 않아 **도입하지 않기로 결정함.**

`SentenceTemplate` → `ResultText` 전환은 UI 코드가 판정 결과를 보고 어느 텍스트를 그릴지 분기하는 것으로 처리한다 (새 필드 불필요).

**`LieText`는 아직 미구현 — 4.2절 참고.**

### 3.7 DT_Characters — `FCharacterDefinition` (신규)

```
CharacterID       FName   행 이름과 동일. DT_Sentences.CharacterID, DT_Photos.CharacterID가 참조
FolderName        FText   태블릿 폴더 표시 이름
FolderSortOrder   int32   폴더 여러 개일 때 나열 순서
```

인물(폴더) 수만큼만 행이 존재 — 진술서·사진 개수와 무관하게 인물 1명당 1행.

**용도 구분**: `DT_Characters`는 "폴더가 몇 개고 뭐라 부르고 무슨 순서냐"(폴더 목록 화면)만 담당하고, `DT_Sentences`/`DT_Photos`는 "그 폴더 안에 실제로 뭐가 들어가냐"(각 항목의 `CharacterID`로 역추적)를 담당한다.

전제 조건(확인 완료): **폴더가 있는 인물은 항상 진술서(Statement 타입 문장)도 있다.**

### 3.8 Subsystem 공개 API (현재)

```cpp
bool GetEvidenceDefinition(FName ObjectID, FEvidenceDefinition& OutDefinition) const;
bool GetEvidenceStateDefinition(FName StateID, FEvidenceStateDefinition& OutState) const;
bool GetPhotoDefinition(FName PhotoID, FPhotoDefinition& OutDefinition) const;
bool GetCharacterDefinition(FName CharacterID, FCharacterDefinition& OutDefinition) const;   // 신규
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

`ValidateSentence`는 이미 다음을 구현하고 있다:
- WordSlots: `bWordOrderMatters`에 따라 위치 고정 검증 또는 다중집합 검증으로 분기
- PhotoSlots: "촬영 여부"뿐 아니라 그 사진의 `PhotoSentenceID`가 가리키는 분석 문장이 실제로 풀렸는지까지 확인 (미완성/스토리 사진을 진술서 증거로 못 쓰게 막음)

`ValidateLoadedDataTables()`가 검증하는 것 (as-is):
- 7개 테이블 모두 Row Name == 내부 ID 필드
- EvidenceDefinition.InitialStateID 존재 및 소속 오브젝트 일치
- EvidenceState.ObjectID 존재, NextStateID(ChangeState 시) 존재/소속 일치, KeywordDocumentID(KeywordSelectionWindow 시) 존재, PhotoID(bCanCapture 시) 존재
- Photo.PhotoSentenceID(있으면) 존재, **Photo.CharacterID 필수 존재** (신규)
- KeywordDocument.KeywordChoices의 ChoiceID 중복, GrantedWordID 존재
- Sentence.CharacterID(있으면) 존재 (신규), WordSlots/PhotoSlots 슬롯 인덱스 범위/중복, CorrectWordID/CorrectPhotoID 존재, RequiredPhotoCount 범위, ResultTextID/ResultText 비어있지 않음
- **Characters 테이블 Row Name 검증** (신규)

### 3.9 Subsystem 런타임 상태 (SaveGame 대상, 현재는 메모리에만 존재)

```cpp
TMap<FGuid, FEvidenceRuntimeState>          EvidenceRuntimeStates;
TMap<FName, FAcquiredWordRecord>            AcquiredWords;
TMap<FName, FCapturedPhotoRecord>           CapturedPhotos;
TMap<FName, FKeywordDocumentRuntimeState>   KeywordDocumentStates;
TMap<FName, FSentenceRuntimeProgress>       SentenceProgress;
```

SaveGame 구현 시 이 5개를 그대로 직렬화. ID만 저장, 표시용 텍스트는 로드 시 DataTable에서 재조회.

## 4. 결정됐지만 아직 미구현 (우선순위 순)

### 4.1 `FPhotoDefinition.GrantedWordIDs` 추가 — 최우선

기획이 "추리용/스토리 사진 촬영 시 오브젝트에 할당된 키워드를 0~n개 획득한다"를 명시적으로 확정함.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
TArray<FName> GrantedWordIDs;
```

- `EWordAcquisitionSource`에 `PhotoCapture` 추가
- `RegisterCapturedPhoto` 성공 시 `GrantedWordIDs`를 순회하며 `AcquireWord(WordID, PhotoCapture, Record.PhotoID)` 호출
- `ValidateLoadedDataTables()`에 각 WordID가 `DT_Words`에 존재하는지 교차검증 추가
- CSV 배열 문법: `(WORD_01_001,WORD_01_002)` (구조체 배열이 아닌 단순 FName 배열이라 괄호 한 겹)

### 4.2 `FSentenceDefinition` 필드 정리

**삭제:**
```cpp
FName ResultTextID;  // 삭제 — 아무도 참조 안 하는 죽은 ID
```
- `ValidateLoadedDataTables()`의 `Sentence->ResultTextID.IsNone()` 체크 삭제
- `ValidateSentence` 시그니처: `FName& OutResultTextID` 파라미터 제거 → `ValidateSentence(SentenceID, Submission, FText& OutResultText)`
- `FOnSentenceSolved` 델리게이트가 `ResultTextID`를 넘긴다면 같이 정리

**추가:**
```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (MultiLine = "true"))
FText LieText;   // 인물이 한 거짓말. 빈칸 없는 고정 문장(SentenceTemplate과 별개).
                 // Statement 타입에만 채움, PhotoAnalysis 타입은 비워둠
```

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
- `UBalhwajeomInvestigationSettings`에 `KeywordChoicesTable` 추가
- Subsystem에 `KeywordChoicesTable` 멤버 + 로드 로직 + `KeywordDocumentID → 선택지(SortOrder 정렬)` 조회 로직
- 신규 공개 API: `GetKeywordDocumentDefinition(KeywordDocumentID, OutDefinition)`, `GetKeywordChoicesForDocument(KeywordDocumentID, OutChoices)`
- 신규 공개 API(선택): `SelectKeywordChoice(KeywordDocumentID, ChoiceID)` — 이미 선택한 ChoiceID 중복 방지(`FKeywordDocumentRuntimeState.SelectedChoiceIDs` 활용) + 성공 시 `AcquireWord` 자동 호출. 현재 `SelectedChoiceIDs`/`KeywordDocumentStates`는 선언만 되어있고 아무도 안 씀(완전히 죽은 상태) — 이 API를 만들어야 실제로 쓰이게 됨
- 검증 로직 재작성: 테이블 전체를 `KeywordDocumentID`별로 그룹핑해서 ChoiceID 중복/GrantedWordID 존재/KeywordDocumentID 참조 유효성 확인. 같은 문서 내 `SortOrder` 중복 검사 권장

### 4.4 `IsSentenceSolved` 공개 API 추가

```cpp
UFUNCTION(BlueprintPure, Category = "Investigation|Sentences")
bool IsSentenceSolved(FName SentenceID) const;
// 구현: const FSentenceRuntimeProgress* P = SentenceProgress.Find(SentenceID); return P != nullptr && P->bSolved;
```

용도: 태블릿 폴더의 파일 아이콘에 `?`/`✓` 표시. 판정(`ValidateSentence`)과 별개의 순수 조회용.

### 4.5 인물별 목록 조회 API (우선순위 낮음)

```cpp
void GetStatementSentencesForCharacter(FName CharacterID, TArray<FSentenceDefinition>& OutSentences) const;
void GetPhotosForCharacter(FName CharacterID, TArray<FPhotoDefinition>& OutPhotos) const;
```

`Photo.CharacterID`가 직접 참조라서(3.4절), `GetPhotosForCharacter`는 이제 `PhotosTable`을 순회하며 `CharacterID`만 직접 비교하면 된다 (문장을 거칠 필요 없음 — 예전 `StatementSentenceID` 설계보다 단순해짐).

소규모 프로토타입 단계에서는 레벨 블루프린트에 표시할 ID 목록을 하드코딩해서 임시로 우회 가능.

## 5. 아직 결론 안 남 (임의로 정하지 말 것)

### 5.1 `FarLabel`의 실제 표시 조건

기획서: "가장 먼 거리에서는 거리별 텍스트를 표시하지 않는다"고 명시. `FarLabel` 필드가 실제로 화면에 노출되는 시점이 불분명함 — `InspectionComponent.MaxDisplayDistance` 경계와 관련해 4단계 구간이 실제로 존재하는지, 아니면 `FarLabel`이 죽은 필드인지 기획 확인 필요. **일단 필드는 유지.**

### 5.2 `bCloseAfterSelection` 삭제 최종 확인

여러 차례 새 CSV 스펙에서 계속 빠져있어 삭제로 거의 확정된 상태지만, 명시적인 "삭제해" 지시는 아직 못 받음. 4.3절 작업 진행 전에 최종 확인 권장.

## 6. 우선순위 체크리스트

- [ ] 4.1 `Photo.GrantedWordIDs` + `EWordAcquisitionSource::PhotoCapture` (핵심 게임플레이 규칙, 최우선)
- [ ] 4.2 `Sentence.ResultTextID` 삭제 + `ValidateSentence` 시그니처 변경
- [ ] 4.2 `Sentence.LieText` 추가
- [ ] 5.2 확인 후 → 4.3 KeywordChoices 별도 테이블 분리 (구조체·Settings·Subsystem 조회/검증·신규 공개 API·`SelectKeywordChoice`)
- [ ] 4.4 `IsSentenceSolved` 공개 API
- [ ] 4.5 인물별 목록 조회 API (필요해지면)
- [ ] 5.1 기획 확인 후 `FarLabel` 처리 방향 결정
- [ ] `Content/Balhwajeom/Data/Investigation/DT_Characters.uasset` 신규 생성, Settings에 연결
- [ ] 위 변경사항을 반영해 `Scripts/Investigation/*.csv` 예시 파일 갱신
- [ ] `BalhwajeomInvestigationSubsystemTest.cpp`에 변경된 필드/시그니처에 맞는 테스트 갱신 (CharacterID 필수 검증, bWordOrderMatters 분기 등)
- [ ] 담당자2(EvidenceActor·상호작용)·담당자3(카메라·사진) 통합은 이 데이터 작업이 끝난 뒤 진행 (아직 미착수 상태, [Investigation-System-Implementation-Handoff.md](./Investigation-System-Implementation-Handoff.md) 12절 참고)
