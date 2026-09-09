# Subsystem 담당 작업 문서

> 기준 문서: [Project-Progress-and-Roadmap.md](../Project-Progress-and-Roadmap.md) 2.1, 5장 P0-3/P1-1
> 담당 범위: `UBalhwajeomInvestigationSubsystem`, `UStoryStateSubsystem`, `BalhwajeomInvestigationSaveGame` (저장 시스템 구현 트랙)

이 문서는 **조사 데이터의 유일한 런타임 소유자인 InvestigationSubsystem, 스토리 진행을 담당하는 StoryStateSubsystem, 그리고 이번 일정의 핵심 작업인 전체 진행 SaveGame**을 다루는 담당자용 문서다.

## 1. 현재 구조 (변경 대상 아님)

```text
DataTable 8종 (정적 정의, 읽기 전용)
        ↓
UBalhwajeomInvestigationSubsystem (GameInstanceSubsystem)
 ├─ EvidenceRuntimeStates   (TMap<FGuid, FEvidenceRuntimeState>)
 ├─ AcquiredWords           (TMap<FName, FAcquiredWordRecord>)
 ├─ CapturedPhotos          (TMap<FName, FCapturedPhotoRecord>)
 ├─ KeywordDocumentStates   (TMap<FName, FKeywordDocumentRuntimeState>)
 └─ SentenceProgress        (TMap<FName, FSentenceRuntimeProgress>)
        ↓ 이벤트(OnEvidenceStateChanged 등)
EvidenceActor / Camera / Tablet / Messenger

UStoryStateSubsystem — Gameplay Tag 기반 별도 진행 상태
```

핵심 파일: [BalhwajeomInvestigationSubsystem.h](../../Source/Balhwajeom/Public/Investigation/BalhwajeomInvestigationSubsystem.h) / `.cpp`, [StoryStateSubsystem.h](../../Source/Balhwajeom/Public/Story/StoryStateSubsystem.h), [BalhwajeomInvestigationSaveGame.h](../../Source/Balhwajeom/Public/Investigation/BalhwajeomInvestigationSaveGame.h)

## 2. 공통 API 상세

아래는 `UBalhwajeomInvestigationSubsystem`의 공개 API다. 변경 시 이 표를 함께 갱신한다.

### 정의 조회 (읽기 전용 — DataTable 행을 그대로 복사해 반환)

| API | 의미 |
|---|---|
| `GetEvidenceDefinition(ObjectID, OutDefinition)` | `DT_EvidenceDefinitions`에서 해당 오브젝트 정의를 조회. 없으면 false |
| `GetEvidenceStateDefinition(StateID, OutState)` | `DT_EvidenceStates`에서 해당 상태 정의를 조회 |
| `GetPhotoDefinition(PhotoID, OutDefinition)` | `DT_Photos`에서 사진 정의를 조회 |
| `GetCharacterDefinition(CharacterID, OutDefinition)` | `DT_Characters`에서 인물 정의를 조회 |
| `GetWordDefinition(WordID, OutDefinition)` | `DT_Words`에서 키워드 정의를 조회 |
| `GetSentenceDefinition(SentenceID, OutDefinition)` | `DT_Sentences`에서 문장 정의를 조회 |
| `GetKeywordDocumentDefinition(KeywordDocumentID, OutDefinition)` | `DT_KeywordDocuments`에서 문서 정의를 조회 |
| `GetKeywordChoicesForDocument(KeywordDocumentID, OutChoices)` | `DT_KeywordChoices`에서 해당 문서 소속 선택지를 `SortOrder` 순으로 정렬해 반환(런타임 상태 없이 정의만 반환) |

### 증거 상호작용 (런타임 상태를 실제로 바꾸는 API)

| API | 의미 |
|---|---|
| `RegisterEvidenceActor(EvidenceInstanceID, ObjectID, OutCurrentStateID)` | 레벨에 배치된 증거 액터를 최초 1회 등록한다. 이미 등록된 인스턴스면 같은 `ObjectID`인지만 확인하고 현재 상태를 그대로 반환(재등록해도 안전). 최초 등록이면 `DT_EvidenceDefinitions.InitialStateID`를 시작 상태로 `EvidenceRuntimeStates`에 새로 만든다 |
| `BeginEvidenceInteraction(EvidenceInstanceID, OutViewData)` | F 상호작용 UI를 띄우기 **전에** "지금 상호작용 가능한가"와 "무엇을 보여줄지"만 조회하는 읽기 전용 호출. `InteractionBehavior=None`이거나 `Once`인데 이미 완료했으면 실패. 이 호출만으로는 상태가 바뀌지 않는다 |
| `CompleteEvidenceInteraction(EvidenceInstanceID, ExpectedStateID)` | UI 표시가 끝난 뒤 실제로 상태를 확정하는 호출. `ExpectedStateID`가 현재 상태와 다르면(그 사이 다른 경로로 상태가 바뀌었으면) 실패해 이중 처리를 막는다. `Once`=완료 플래그 기록, `Repeatable`=항상 성공, `ChangeState`=`NextStateID`로 전환하며 `OnEvidenceStateChanged`를 브로드캐스트 |

### 키워드

| API | 의미 |
|---|---|
| `AcquireWord(WordID, SourceType, SourceID)` | 키워드를 1회 지급한다. 이미 보유 중이거나 `DT_Words`에 없는 ID면 실패. 성공 시 `OnWordAcquired` 브로드캐스트. `SourceType`/`SourceID`는 "어디서 얻었는지" 기록용이며 판정 로직에는 영향을 주지 않는다 |
| `HasAcquiredWord(WordID)` | 보유 여부만 조회 |
| `GetAcquiredWords(OutWords)` | 보유한 전체 키워드를 획득 시간순으로 반환 |
| `GetAcquiredWordsForCharacter(CharacterID, OutWords)` | 보유 키워드 중 `RelatedCharacterIDs`에 해당 인물이 포함된 것만 필터링(태블릿 인물 폴더 표시용) |
| `SelectKeywordChoice(KeywordDocumentID, ChoiceID, SourceType)` | F 조사의 키워드 선택창에서 하나를 고를 때 호출. 같은 문서에서 같은 선택지를 두 번 고르면 실패(중복 방지)하고, 성공하면 해당 선택지의 `GrantedWordID`를 `AcquireWord`로 지급한다 |

### 사진

| API | 의미 |
|---|---|
| `HasCapturedPhoto(PhotoID)` | 촬영 여부만 조회 |
| `RegisterCapturedPhoto(Record)` | 카메라 촬영 성공 시 호출하는 등록 함수. `PhotoID` 중복, 증거 런타임 상태 불일치, `DT_Photos`에 없는 ID 중 하나라도 해당하면 실패. 성공하면 즉시 `SavePersistentPhotoGallery`로 저장하고(저장 자체가 실패하면 등록도 롤백), `GrantedWordIDs`의 키워드를 전부 지급한 뒤 `OnPhotoCaptured`를 브로드캐스트 |
| `GetCapturedPhotos(OutPhotos)` | 촬영된 사진 전체를 촬영 시간순으로 반환 |

### 문장 / 진술서

| API | 의미 |
|---|---|
| `ValidateSentence(SentenceID, Submission, OutResultText)` | 사진 분석 퍼즐과 진술서 반증에 공통으로 쓰이는 채점 함수. `WordSlots`(고정 순서/무순서 그룹 모두 지원)와 `PhotoSlots`(`RequiredPhotoCount` 이상 매칭)가 모두 조건을 만족하고 `ResultText`가 비어 있지 않아야 성공한다. 성공 시 해결 상태를 기록하고, **최초 해결일 때만** `OnSentenceSolved`를 브로드캐스트한다. 주의: 사진 정답 슬롯은 "그 사진의 분석 문장(`PhotoSentenceID`)이 이미 풀렸는지"까지 확인하므로, 촬영만 하고 분석을 풀지 않은 사진은 진술서 정답 사진으로 인정되지 않는다 |
| `IsSentenceSolved(SentenceID)` | 해결 여부만 조회 |
| `GetStatementSentencesForCharacter(CharacterID, OutSentences)` | `DT_Sentences` 중 `SentenceType=Statement`이고 해당 인물 소속인 문장만 반환(태블릿 진술서 후보 목록) |
| `GetPhotosForCharacter(CharacterID, OutPhotos)` | `DT_Photos` 중 해당 인물 소속 사진 **정의**를 전부 반환한다. 촬영 여부와 무관하게 정의를 반환하므로, 실제 촬영된 사진만 보여주려면 호출 측(태블릿)에서 `HasCapturedPhoto`와 조합해 걸러야 한다 |

### 이벤트(Delegate)

| 이벤트 | 브로드캐스트 시점 |
|---|---|
| `OnEvidenceStateChanged(EvidenceInstanceID, PreviousStateID, NewStateID)` | `ChangeState` 상호작용이 완료된 순간. EvidenceActor가 이 이벤트로 라벨/메시 상태를 갱신한다 |
| `OnWordAcquired(WordRecord)` | `AcquireWord`가 새로 성공한 순간 |
| `OnPhotoCaptured(PhotoRecord)` | `RegisterCapturedPhoto`가 새로 성공한 순간 |
| `OnSentenceSolved(SentenceID)` | 문장이 **처음으로** 풀린 순간(재제출 시에는 브로드캐스트되지 않는다) |

### 저장 (현재 private, 사진 메타데이터 전용 — 이번 일정에서 확장할 대상)

| 함수 | 의미 |
|---|---|
| `LoadPersistentPhotoGallery()` | `Initialize`에서 자동 호출. SaveGame 슬롯(`BalhwajeomInvestigation`, 자동화 테스트 중에는 별도 슬롯)에서 촬영 사진 메타데이터를 불러온다. 이미지 파일 경로·크기까지 검증해 깨진 레코드는 무시한다 |
| `SavePersistentPhotoGallery()` | `RegisterCapturedPhoto` 성공 시 자동 호출. 현재 `CapturedPhotos` 전체를 슬롯에 저장한다 |
| `ShouldPersistPhotoGallery()` | 실제 게임 월드(PIE/패키지)일 때만 true. 에디터 프리뷰나 자동화 테스트에서 세이브 파일이 오염되지 않도록 막는 가드 |

## 3. 변경/구현 필요 사항 — 전체 진행 SaveGame (이번 일정의 핵심)

로드맵 P1-1 기준, 현재 영속 저장은 사진 메타데이터뿐이다. 다음을 `BalhwajeomInvestigationSaveGame`에 추가하고 Subsystem에 로드/저장 API를 연결해야 한다.

- [ ] `EvidenceRuntimeStates` 저장/복원 (증거 인스턴스 ID ↔ 현재 상태 ID)
- [ ] `AcquiredWords` 저장/복원
- [ ] `KeywordDocumentStates` 저장/복원
- [ ] `SentenceProgress` 저장/복원
- [ ] `StoryStateSubsystem`의 Gameplay Tag 진행 상태 저장/복원 (Investigation SaveGame과 같은 시점에 저장되거나, 별도 슬롯이면 두 시스템 간 저장 시점 동기화 방식을 정한다)
- [ ] 저장 데이터 버전 필드와 마이그레이션 정책(향후 필드 추가 시 구버전 세이브 호환 방법) 최소 1개 정의
- [ ] 사진 파일(`Saved/Investigation/Photos`)과 SaveGame 메타데이터의 정합성 유지(태블릿 트랙의 사진 수명주기 작업과 인터페이스 공유)

## 4. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: SaveGame 데이터 구조 설계 확정(필드, 버전), EvidenceRuntimeStates/AcquiredWords 저장·복원 1차 구현
- **2단계(9/13~9/15, M2)**: KeywordDocumentStates/SentenceProgress/StoryState 저장·복원 완료, 종료→재실행 시나리오 수동 검증
- **3단계(9/16~9/17, M3)**: 다른 트랙(카메라/상호작용/태블릿) 통합 후 발생하는 저장 불일치 수정, 회귀 테스트
- **4단계(9/18, M4)**: 최종 QA 대응(저장/로드 관련 버그만 우선 처리)

## 5. 완료 조건

- 게임 종료 후 재실행해도 증거 상태, 키워드, 사진, 문장 풀이, 스토리 진행이 서로 모순 없이 복원된다.
- `Balhwajeom.Investigation`, Story 자동화 테스트가 저장/복원 케이스를 포함해 통과한다.
- 저장 데이터에 최소 1개의 버전 필드가 있다.

## 6. 주의 사항

- Investigation C++ 코드와 DataTable `.uasset`은 이 트랙과 [DataTable-Handoff.md](./DataTable-Handoff.md) 담당자가 동시에 건드리는 영역이므로, 헤더/구조체 변경 전 상호 공지한다.
- Subsystem은 "플레이 중 바뀌는 값의 단일 기준"이라는 원칙을 유지한다. 카메라·태블릿·상호작용 쪽에서 상태를 별도로 들고 있지 않도록 한다.
