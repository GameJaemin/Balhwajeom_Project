# DataTable 담당 작업 문서

> 기준 문서: [Project-Progress-and-Roadmap.md](../Project-Progress-and-Roadmap.md) 2.1~2.2, 5장 P0-2, P1-4
> 담당 범위: `Scripts/Investigation/*.csv` ↔ `Content/Balhwajeom/Data/Investigation/DT_*.uasset` 8종과 `BalhwajeomInvestigationSettings`

이 문서는 **본편 시나리오 데이터를 8개 DataTable에 실제로 채워 넣는 작업**을 맡은 담당자용 체크리스트다. 코드 구조를 바꾸는 작업이 아니라, 이미 만들어진 스키마에 진짜 데이터를 넣고 검증하는 작업이 중심이다.

## 1. 현재 구조 (변경 대상 아님)

```text
Scripts/Investigation/*.csv  (원본, 담당자가 직접 편집)
        ↓ 에디터에서 재임포트
Content/Balhwajeom/Data/Investigation/DT_*.uasset  (8종)
        ↓ BalhwajeomInvestigationSettings에 등록된 경로
UBalhwajeomInvestigationSubsystem이 로드/검증
```

| CSV / DataTable | 정의 파일 | 역할 |
|---|---|---|
| `DT_EvidenceDefinitions` | [EvidenceDefinitions.h](../../Source/Balhwajeom/Public/Investigation/EvidenceDefinitions.h) | 증거 오브젝트와 최초 상태 |
| `DT_EvidenceStates` | 위 파일 내 상태 구조체 | 상태별 상호작용·거리 문구·촬영 조건 |
| `DT_Words` | [WordDefinitions.h](../../Source/Balhwajeom/Public/Investigation/WordDefinitions.h) | 키워드 정의와 인물 폴더 소속 |
| `DT_Photos` | [PhotoDefinitions.h](../../Source/Balhwajeom/Public/Investigation/PhotoDefinitions.h) | 사진, 인물, 분석 문장, 지급 키워드, 스토리 정보 |
| `DT_KeywordDocuments` | WordDefinitions.h 내 문서 구조체 | F 조사 문서 본문 |
| `DT_KeywordChoices` | 위와 동일 | 문서별 선택지와 지급 키워드 |
| `DT_Sentences` | [SentenceDefinitions.h](../../Source/Balhwajeom/Public/Investigation/SentenceDefinitions.h) | 사진 분석 문장과 진술서 반증 조건(LieText 포함) |
| `DT_Characters` | [CharacterDefinitions.h](../../Source/Balhwajeom/Public/Investigation/CharacterDefinitions.h) | 태블릿 인물 폴더 이름과 정렬 순서 |

컬럼별 상세 의미와 다른 테이블과의 참조 관계는 [2. 테이블별 컬럼 상세](#2-테이블별-컬럼-상세)에서 확인한다.

원칙:
- CSV가 원본, `.uasset`은 재임포트 산출물이다. `.uasset`을 직접 편집하지 말 것.
- Row Name과 행 내부 ID 필드는 반드시 동일해야 한다.
- 8개 테이블 중 하나라도 담당자를 나누지 말고, CSV 재임포트는 이 문서 담당자 1명이 고정으로 수행한다(다른 트랙과의 `.uasset` 충돌 방지).

## 2. 테이블별 컬럼 상세

CSV 헤더 = Row Struct의 `UPROPERTY` 이름이다. Row Name(CSV 첫 컬럼)은 각 테이블의 ID 컬럼(예: `ObjectID`, `StateID`)과 반드시 같은 값이어야 하며, 다르면 자동화 검증(`ValidateLoadedDataTables`)에서 오류로 잡힌다. `(참조)`로 표시한 컬럼은 다른 DataTable의 ID를 가리키며, 존재하지 않는 ID를 넣으면 마찬가지로 검증에서 걸러진다.

### DT_EvidenceDefinitions — 증거 오브젝트 정의

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `ObjectID` | Name (Row Name) | 증거 오브젝트 고유 ID. `ABalhwajeomEvidenceActor::ObjectID`가 이 값으로 등록된다 |
| `ObjectName` | Text | 화면에 노출되는 오브젝트 이름 |
| `InitialStateID` | Name (참조: `DT_EvidenceStates.StateID`) | 게임 시작 시 이 오브젝트가 갖는 최초 상태. `DT_EvidenceStates`에서 같은 `ObjectID`를 가진 행이어야 한다 |

### DT_EvidenceStates — 상태별 상호작용·거리·촬영 조건

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `StateID` | Name (Row Name) | 상태 고유 ID |
| `ObjectID` | Name (참조: `DT_EvidenceDefinitions.ObjectID`) | 이 상태가 속한 증거 오브젝트 |
| `StateName` | Text | 상태 이름(에디터/디버그 표기용) |
| `InteractionBehavior` | Enum(`None`/`Once`/`Repeatable`/`ChangeState`) | F 상호작용 동작 방식. `None`=상호작용 불가, `Once`=1회만 성공, `Repeatable`=매번 성공, `ChangeState`=성공 시 `NextStateID`로 상태 전환 |
| `InteractionPresentation` | Enum(`None`/`SimpleText`/`KeywordSelectionWindow`) | 상호작용 결과를 어떻게 보여줄지. `SimpleText`=문구만 표시, `KeywordSelectionWindow`=`KeywordDocumentID` 문서의 선택창을 연다 |
| `NextStateID` | Name (참조: `DT_EvidenceStates.StateID`) | `ChangeState`일 때 전환할 다음 상태. 같은 `ObjectID` 소속이어야 한다 |
| `InteractionText` | Text | `SimpleText`일 때 보여줄 문구 |
| `KeywordDocumentID` | Name (참조: `DT_KeywordDocuments.KeywordDocumentID`) | `KeywordSelectionWindow`일 때 열 문서 |
| `FarLabel` / `MidLabel` / `NearLabel` | Text | 거리 단계별(멀리/중간/가까이) 표시 라벨. `NearLabel`은 F 조사와 별개로, 가장 가까운 거리에서 상시 표시되는 관찰 정보 문구다(2026-09-09 기준 `ObservationText`에서 개명) |
| `bCanCapture` | Bool | 카메라로 촬영 가능한 상태인지 |
| `PhotoID` | Name (참조: `DT_Photos.PhotoID`) | `bCanCapture`가 true일 때 촬영 성공 시 등록될 사진. true인데 유효한 `PhotoID`가 없으면 검증 오류 |
| `PreferredFocusDistance` / `FocusDistanceTolerance` | Float | 카메라 초점 판정 기준 거리와 허용 오차(1배율 기준, cm) |
| `bScaleFocusDistanceWithZoom` | Bool | 줌 배율에 따라 위 초점 거리 기준을 함께 스케일할지 여부 |

### DT_Words — 키워드 정의

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `WordID` | Name (Row Name) | 키워드 고유 ID |
| `DisplayWord` | Text | 화면에 표시되는 단어 |
| `Description` | Text | 키워드 설명 |
| `RelatedCharacterIDs` | Name 배열 (참조: `DT_Characters.CharacterID`) | 이 키워드가 노출되고 진술서 정답 후보로 쓰일 수 있는 인물 폴더들. 진술서(`Statement`) 문장의 정답 키워드는 반드시 해당 인물이 이 목록에 포함되어 있어야 검증을 통과한다 |
| `bUnlockedByDefault` | Bool | true면 게임 시작과 동시에 획득 상태로 시작(`InitializeDefaultWords`) |

### DT_KeywordDocuments — F 조사 문서 본문

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `KeywordDocumentID` | Name (Row Name) | 문서 고유 ID |
| `DocumentText` | Text | F 조사 시 표시되는 문서 본문 |
| `KeywordChoices` / `bCloseAfterSelection` | (Deprecated) | 구조 재설계 이전 필드. 신규 데이터에는 사용하지 말고 선택지는 반드시 `DT_KeywordChoices`에 작성한다 |

### DT_KeywordChoices — 문서별 선택지와 지급 키워드

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `ChoiceID` | Name (Row Name) | 선택지 고유 ID |
| `KeywordDocumentID` | Name (참조: `DT_KeywordDocuments.KeywordDocumentID`) | 이 선택지가 속한 문서 |
| `DisplayText` | Text | 선택지 문구 |
| `GrantedWordID` | Name (참조: `DT_Words.WordID`) | 이 선택지를 고르면 지급되는 키워드 |
| `SortOrder` | Int | 같은 문서 내 표시 순서(같은 문서에서 값이 중복되면 경고 로그가 남는다) |

### DT_Photos — 사진, 인물, 분석 문장, 지급 키워드, 스토리 정보

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `PhotoID` | Name (Row Name) | 사진 고유 ID |
| `PhotoName` | Text | 사진 이름 |
| `DescriptionSource` | Enum(`None`/`NearLabel`/`InteractionText`/`Custom`) | 태블릿에 보여줄 사진 설명을 어디서 가져올지. `NearLabel`/`InteractionText`는 촬영된 증거 상태(`DT_EvidenceStates`)의 같은 이름 필드를 재사용하고, `Custom`이면 아래 필드를 직접 사용 |
| `CustomDescription` | Text | `DescriptionSource=Custom`일 때 사용하는 설명 |
| `PhotoSentenceID` | Name (참조: `DT_Sentences.SentenceID`, `SentenceType=PhotoAnalysis`) | 이 사진의 분석 퍼즐로 연결되는 문장 |
| `EvidenceSentenceID` | Name (참조: `DT_Sentences.SentenceID`, `SentenceType=PhotoAnalysis`, 선택) | 이 사진을 **어느 진술서에서든 증거로 제출할 때** 추가로 풀어야 하는 빈칸 문제. 사진 고유 속성이라 어느 진술서·어느 슬롯에서 쓰이든 항상 같은 문장이 뜬다. 비어있으면 추가 문제 없이 기존처럼 판정 |
| `CharacterID` | Name (참조: `DT_Characters.CharacterID`) | 이 사진이 표시될 태블릿 인물 폴더 |
| `GrantedWordIDs` | Name 배열 (참조: `DT_Words.WordID`) | 촬영 성공 시 1회 지급되는 키워드들 |
| `WorldStoryLines` | Text 배열 | 촬영 직후 및 태블릿에서 사진을 다시 열 때 순서대로 보여줄 월드 스토리 대사 |
| `StoryVoice` | Sound 참조 | `WorldStoryLines`와 함께 재생할 내레이션 음성 |

### DT_Sentences — 사진 분석 문장과 진술서 반증 조건

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `SentenceID` | Name (Row Name) | 문장 고유 ID |
| `SentenceType` | Enum(`PhotoAnalysis`/`Statement`) | `PhotoAnalysis`=사진 분석 퍼즐, `Statement`=태블릿 진술서 반증용 |
| `CharacterID` | Name (참조: `DT_Characters.CharacterID`) | `Statement` 타입일 때 소속 인물(`PhotoAnalysis`는 비워둔다) |
| `LieText` | Text | 진술서에서 반증 대상이 되는 인물의 거짓 진술 문구 |
| `SentenceTemplate` | Text | 빈칸이 있는 문장 템플릿(퍼즐 UI 표시용) |
| `WordSlots` | 구조체 배열(`SlotIndex` 0~4, `CorrectWordID`(참조: `DT_Words`), `OrderGroup`) | 빈칸별 정답 키워드. `OrderGroup=0`이면 그 슬롯 위치가 고정, 0이 아니면 같은 그룹끼리는 순서 상관없이 채워도 정답으로 인정(`ValidateSentence` 참고) |
| `PhotoSlots` | 구조체 배열(`SlotIndex` 0~1, `CorrectPhotoID`(참조: `DT_Photos`)) | 진술서에 첨부해야 할 정답 사진. **분석이 이미 풀린 사진만** 후보로 인정되고, 그 사진에 `EvidenceSentenceID`(`DT_Photos` 참고)가 있으면 그 문장도 풀려야 정답으로 인정된다 |
| `RequiredPhotoCount` | Int (0~`PhotoSlots` 수) | 정답 인정에 필요한 최소 사진 매칭 수 |
| `ResultText` | Text | 정답 제출 성공 시 보여줄 결과 문구. **비어 있으면 검증 자체가 실패로 처리**되므로 반드시 채운다 |
| `DesignerNote` | Text | 기획자 메모(런타임에서 사용되지 않음) |

### DT_Characters — 태블릿 인물 폴더 이름과 정렬 순서

| 컬럼 | 타입 | 의미 |
|---|---|---|
| `CharacterID` | Name (Row Name) | 인물 고유 ID. `DT_Words`/`DT_Photos`/`DT_Sentences`가 이 ID로 인물을 참조한다 |
| `FolderName` | Text | 태블릿 인물 폴더에 표시될 이름 |
| `FolderSortOrder` | Int | 폴더 목록 정렬 순서 |

## 3. 변경 필요 사항

- [ ] 샘플 데이터(`CHAPTER_01`, `SISTER` 등 프로토타입용 예시)를 실제 시나리오 데이터로 교체하거나, 실 데이터와 분리한다.
- [ ] 본편에 필요한 인물·증거·상태·키워드·사진·문서·문장 ID를 마스터 시트로 먼저 확정한 뒤 CSV에 반영한다(다른 트랙이 이 ID를 참조하므로 선(先)확정 필수).
- [ ] `DT_Photos`의 인물 폴더 소속(`CharacterID`), 지급 키워드(`GrantedWordIDs`), 월드 스토리 문장, 음성 참조 필드를 실제 콘텐츠 기준으로 검수한다.
- [x] `DT_OutputTexts.uasset`은 최신 구조에서 참조되지 않아 제거함(2026-09-09).
- [ ] `Scripts/Investigation/README.md`의 "7종" 표기를 실제 8종으로 수정한다.
- [ ] `DT_KeywordDocuments`의 구형 중첩 선택지 필드(현재는 `DT_KeywordChoices`로 분리됨) 잔재가 있으면 제거한다.

## 4. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: 마스터 ID 목록 확정 → 8개 CSV에 본편 데이터 입력 → 에디터 재임포트 → `Balhwajeom.Investigation` 자동화로 1차 교차 검증
- **2단계(9/13~9/15, M2)**: 사진·문장·키워드 트랙(카메라/상호작용/태블릿)에서 요청하는 누락 ID 보강, README 정리
- **3단계(9/16~9/17, M3)**: 전체 통합 이후 재검증, 다른 트랙이 발견한 데이터 버그 반영
- **4단계(9/18, M4)**: 최종 DataTable 교차 검증 통과 여부만 확인(QA 트랙에 결과 전달)

## 5. 완료 조건

- 본편에 필요한 모든 ID가 마스터 시트와 8개 DataTable에 존재한다.
- 8개 DataTable 교차 검증(자동화 테스트)이 오류 없이 통과한다.
- CSV와 `.uasset`이 항상 동일한 최신 재임포트 상태를 유지한다.
- Row Name 불일치, 참조 누락(예: 존재하지 않는 CharacterID/WordID 참조) 0건.
