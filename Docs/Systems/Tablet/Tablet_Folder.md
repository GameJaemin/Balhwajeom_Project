# 태블릿 담당 작업 문서 — 인물 폴더 (사진 · 진술서)

> 기준 문서: [Project-Progress-and-Roadmap.md](../../Project-Progress-and-Roadmap.md) 2.3(태블릿과 추리), 5장 P0-2/P1-2
> 담당 범위: `UBalhwajeomTabletWidget`의 `PersonFolder` 페이지 — 사진/인물 폴더 조회, 사진 분석 퍼즐, 진술서(진술 반증)
> 같은 태블릿 폴더의 다른 문서: [Tablet_Browser.md](./Tablet_Browser.md), [Tablet_messenger.md](./Tablet_messenger.md)

핵심 파일: [BalhwajeomTabletWidget.h](../../../Source/Balhwajeom/Public/Tablet/BalhwajeomTabletWidget.h) / `.cpp`, `WBP_Tablet.uasset`(`Content/Balhwajeom/UI/Tablet`)

`UBalhwajeomTabletWidget`은 태블릿의 5개 페이지(`ETabletPage`: `Home`, `PersonFolder`, `Messenger`, `Internet`, `Memo`)를 한 위젯에서 전환하는 공용 클래스다. 이 헤더는 [Tablet_Browser.md](./Tablet_Browser.md), [Tablet_messenger.md](./Tablet_messenger.md) 담당자와 공유되므로, `UBalhwajeomTabletWidget.h`를 직접 수정할 때는 다른 두 담당자에게 미리 공지한다.

## 1. 인물 폴더 배치 (홈 화면)

현재 상태: **완료(2026-09-09)**. 기존에는 `EFamilyMember`(Sister/Brother/Mother) 고정 enum과 `BTN_Sister`/`BTN_Brother`/`BTN_Mother` 3개 고정 버튼으로 홈 화면을 구성했으나, `DT_Characters` 기반 완전 동적 생성으로 교체했다.

- `UBalhwajeomInvestigationSubsystem::GetAllCharacterDefinitions`가 `DT_Characters`의 모든 행을 `FolderSortOrder`(동률이면 `CharacterID` 사전순)로 정렬해 반환한다.
- `UBalhwajeomTabletWidget::RefreshHomeFolders()`가 `NativeOnInitialized` 시 이 목록으로 홈 화면의 `WB_PersonFolders`(WrapBox)를 채운다. 각 항목은 새 위젯 `UBalhwajeomTabletFolderButton`(공용 폴더 아이콘 + `FolderName` 텍스트)이며, 클릭 시 `HandleHomeFolderSelected` → `ShowFolder(CharacterID)`로 인물 폴더 페이지를 연다.
- `ActiveCharacterID`는 이제 enum이 아니라 `FName`(DT_Characters의 CharacterID)이다. `GetActiveCharacterID()`가 public BlueprintPure로 노출된다.
- 폴더 버튼 아이콘은 인물별 전용 아트가 아니라 **공용 폴더 아이콘 1개**를 쓰기로 결정함(사용자 확인 완료). `UBalhwajeomTabletWidget::DefaultFolderIcon`(EditDefaultsOnly)에 텍스처를 지정한다.
- `WBP_Tablet` 생성 스크립트([TabletWidgetBlueprintLibrary.cpp](../../../Source/BalhwajeomEditor/Private/Tablet/TabletWidgetBlueprintLibrary.cpp)의 `BuildHomePage()`)도 3개 고정 아이콘 버튼 대신 `WB_PersonFolders` WrapBox 하나만 배치하도록 수정했다.

인물이 늘어나면 **코드 수정 없이 `DT_Characters`에 행만 추가**하면 홈 화면 폴더가 자동으로 늘어난다.

### 남은 수동 작업 (에디터에서 진행 필요)

- [x] `WBP_Tablet` 재생성 완료(2026-09-09, `redesign_tablet_widget_blueprint.py` 헤드리스 실행). `BTN_Sister`/`BTN_Brother`/`BTN_Mother`/`BTN_EvidenceStatement`/`TXT_EvidenceStatement`/`TXT_FolderSubtitle`는 삭제됐고, `WB_PersonFolders`/`IMG_PopupPhoto`/`IMG_FolderTitleIcon`/`BTN_FolderClose`가 새로 생성됨.
- [x] `WBP_Tablet` Class Defaults의 `Default Folder Icon`에 `/Game/Balhwajeom/UI/Tablet/Folder` 텍스처 적용 완료(2026-09-09, `set_tablet_folder_icon.py`). 홈 화면 폴더 버튼과 폴더 창 타이틀 아이콘에 함께 쓰인다.
- [x] `TABLET_SMOKE` 자동화(`RunTabletWidgetSmokeTest`) 재실행 통과 확인(2026-09-09). `Balhwajeom.Investigation` 자동화는 별도로 재실행 필요.
- [x] Chapter 1(여동생) 실 데이터로 `DT_Characters` 포함 8종 DataTable 교체 완료(2026-09-09) — [DataTable-Handoff.md](../DataTable-Handoff.md) 참고. 현재는 `CHARACTER_SISTER` 1건뿐이라 홈 화면 폴더도 1개만 보인다.

## 2. 사진 / 진술서 폴더 타일 (완료, 2026-09-09)

`ShowFolder(CharacterID)`가 페이지를 연 뒤 `RefreshFolderContents`가 두 곳을 채운다:
- `WB_EvidencePhotos`(WrapBox): 촬영된 사진 타일들만(진술서는 더 이상 여기 없음). 각 타일은 ~170x170(`SizeBox`)로 홈 화면 폴더 아이콘과 거의 같은 크기다. `UBalhwajeomTabletWidget::GetOrLoadCapturedPhotoTexture(PhotoID)`가 `FImageUtils::ImportFileAsTexture2D`로 촬영된 PNG(`FCapturedPhotoRecord::ImageRelativePath`)를 런타임에 디코드해 실제 썸네일로 보여준다(디코드 결과는 `CapturedPhotoTextureCache`에 캐시).
- `SB_StatementTile`(SizeBox, 170x170 고정): 폴더 창 **하단 중앙**에 단독으로 배치된 진술서 타일(있는 경우만 표시, 없으면 `Collapsed`). 사진과 같은 위젯(`UBalhwajeomTabletPhotoButton`)을 재사용하지만 썸네일 없이 텍스트만(`"{FolderName} 진술서"`) 표시하고 `HandleStatementTileSelected(SentenceID)`로 연결된다 — 인물별 진술서는 현재 1개까지만 지원(`VisibleStatementIDs[0]`).

두 타일 종류 모두 (2026-09-09 재수정) 라벨 앞의 ✓/? 표시는 제거했고(사용자 확인: 불필요), `SetAutoWrapText(false)` + `SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis)`로 실제 윈도우 탐색기처럼 한 줄로 표시하고 넘치면 "..."으로 잘리게 했다. 버튼 자체의 기본 회색 배경/패딩도 `MakeButtonTransparent()`(스타일을 `NoDrawType` 브러시 + 0 패딩으로 교체)로 제거해, 사진 썸네일 뒤에 남던 회색 박스를 없앴다(윈도우 사진 아이콘처럼 사진만 보이도록).

사진을 클릭하면 `OpenPhoto`가 `Photo.PhotoSentenceID` 존재 여부로 화면을 분기한다(둘 다 팝업 이미지(`IMG_PopupPhoto`)에 같은 캐시된 텍스처를 표시):
- **분석 문장 있음**(`SentenceType=PhotoAnalysis`): 문장 빈칸(`SentenceTemplate`/해결 시 `ResultText`)과 함께 `PreparePuzzle`이 아래 3절의 드래그 앤 드롭 퍼즐을 띄운다.
- **분석 문장 없음**(자연어만) 또는 이미 해결된 문장: `CustomDescription`/`WorldStoryCues.Text`/`ResultText`를 그대로 보여주고, 퍼즐 컨트롤은 띄우지 않는다. `WorldStoryCues`가 비어 있는 기존 데이터만 `WorldStoryLines`를 대체 사용한다.

`ShowPopup(Title, Body, PhotoTexture = nullptr)`가 팝업의 단일 진입점이다. `PhotoTexture`가 없으면(예: 진술서 팝업) `IMG_PopupPhoto`를 `Collapsed` 처리한다.

**획득 키워드 표시 위치(완료, 2026-09-09)**: 폴더 페이지 자체에는 없고, **사진 또는 진술서를 열었을 때만** 팝업 안(`WB_PuzzleWords`)에 보인다. `ShowPopup`이 항상 `RefreshAcquiredWordsDisplay()`를 호출해 그 인물의 획득 키워드 전체를 드래그 가능한 칩(`UBalhwajeomTabletWordChip`)으로 채운다(해결 여부·문장 유무와 무관하게 항상 표시). 이후 `PreparePuzzle`→`RefreshPuzzleControls`가 실행되면(분석 문장이 있고 미해결일 때) 같은 `WB_PuzzleWords`를 퍼즐용 후보 목록으로 덮어써 대체한다.

**폴더 창 UI(완료, 2026-09-09)**: 실제 윈도우 탐색기 창처럼 보이도록 폴더 페이지 상단을 "타이틀바" 구조로 바꿨다 — 사용자 확인 사항: 색상은 태블릿 기존 세피아톤 유지(구조만 윈도우식), 주소창/검색창은 만들지 않음, 아이콘 배치는 기존 그리드(큰 아이콘) 뷰 유지.
- 왼쪽: `IMG_FolderTitleIcon`(작은 폴더 아이콘, `DefaultFolderIcon` 공유) + `TXT_FolderTitle`.
- 오른쪽: `BTN_FolderClose`("×" 버튼, 실제로는 기존 `HandleBackClicked`/`NavigateBack()`을 그대로 호출).
- 주소창/검색창은 만들지 않았다(사용자 확인).
- 폴더 아이콘 텍스처는 `/Game/Balhwajeom/UI/Tablet/Folder`를 `DefaultFolderIcon`에 적용해 홈 화면 폴더 버튼·타이틀바 아이콘에 공용으로 쓴다.

변경 필요 사항:
- [ ] 사진 갤러리의 읽음/안읽음 표시(`bViewedInTablet`, 로드맵 P1-2)를 이 페이지에 반영할지 확정.
- [ ] 인물별 진술서가 2개 이상 필요해지면 `VisibleStatementIDs[0]` 고정 대신 여러 타일을 만들도록 확장한다.

## 3. 퍼즐 — 드래그 앤 드롭 (문장 빈칸 + 증거 사진), 진술서 반증 (완료, 2026-09-09)

기존의 "후보 목록 클릭 → 다음 빈 슬롯에 순서대로 채움" 방식(`AvailablePuzzleWordIDs`/`NextWordSlotCursor` 커서)을 걷어내고, **드래그 앤 드롭**으로 완전히 교체했다.

**문장 빈칸(`WordSlots`)**: `PreparePuzzle`→`RefreshPuzzleControls`가 실행되면 `BuildSentenceBuilder(Sentence)`가 `SentenceTemplate`을 `[]` 기준으로 나눠, 고정 텍스트 구간은 `UTextBlock`으로, 각 `[]`(즉 `WordSlots`의 `SlotIndex` 순서)는 드롭 가능한 `UBalhwajeomTabletSentenceBlank`(`UUserWidget`, `NativeOnDrop` 재정의)로 `WB_SentenceBuilder`(WrapBox)에 채워 넣는다. `WB_PuzzleWords`의 각 획득 키워드는 `UBalhwajeomTabletWordChip`(`UUserWidget`, `NativeOnMouseButtonDown`+`NativeOnDragDetected`로 `FReply::DetectDrag`를 사용해 드래그를 시작)이며, 드래그 페이로드는 `UBalhwajeomWordDragDropOperation::WordID`로 전달된다. 칩을 빈칸 위에 놓으면 `UBalhwajeomTabletSentenceBlank::NativeOnDrop`이 `OnBlankDropped`를 브로드캐스트하고, `UBalhwajeomTabletWidget::HandleSentenceBlankDropped`가 `ActiveSubmission.SubmittedWords`에 (정답 여부와 무관하게) 채워 넣는다.

**증거 사진 슬롯(`PhotoSlots`, "완성 사진" 재도입)**: 진술서(`SentenceType=Statement`)가 `PhotoSlots`를 요구하면(현재 본편 Chapter 1 데이터는 전부 `RequiredPhotoCount=0`이라 실제로는 비어 있는 상태 — 향후 데이터에서 채워지면 자동 노출됨), `BuildPhotoSlots(Sentence)`가 `FSentencePhotoSlot`마다 드롭 대상 `UBalhwajeomTabletPhotoSlot`을 `WB_PhotoSlots`에 채운다. 드래그 후보는 `WB_PuzzlePhotos`에 `UBalhwajeomTabletPhotoChip`으로 나열되는데, **자기 자신의 분석 문장(`PhotoSentenceID`)이 이미 해결된 촬영 사진만** 후보로 노출된다(`RefreshPuzzleControls`에서 `Investigation->GetCapturedPhotos()` 전체를 순회하며 `!PhotoSentenceID.IsNone() && IsSentenceSolved(PhotoSentenceID)` 필터). 칩을 슬롯에 놓으면 `HandlePhotoSlotDropped`가 `ActiveSubmission.SubmittedPhotos`에 채워 넣는다. 정답 판정(사진이 맞는 증거인지, `EvidenceSentenceID` 요구 여부 등)은 전부 `InvestigationSubsystem::ValidateSentence`가 담당한다.

**정답 판정 타이밍(2026-09-09 수정)**: 예전에는 빈칸에 잘못된 키워드를 놓을 때마다 즉시 "잘못된 증거인 것 같다"가 떴지만, **사용자 피드백에 따라 모든 빈칸(단어+사진)이 채워진 뒤에만** 판정하도록 바꿨다. `HandleSentenceBlankDropped`/`HandlePhotoSlotDropped`는 이제 드롭된 값을 그냥 채워 넣기만 하고, `EvaluatePuzzleIfComplete()`가 `ActiveSubmission.SubmittedWords.Num() == WordSlots.Num() && SubmittedPhotos.Num() == PhotoSlots.Num()`일 때만 `ValidateActivePuzzle(false)`를 호출한다. `ValidateActivePuzzle`은 `InvestigationSubsystem::ValidateSentence`가 실패를 반환하면 그때 `TXT_PuzzleFeedback`("잘못된 증거인 것 같다")을 띄우고, 성공하면 `ResultText`를 보여주며 퍼즐을 닫는다. `SentenceType=Statement`는 여전히 다 채워졌어도 자동 판정하지 않고 `BTN_StatementSubmit`("자백 반증") 클릭 → `HandleStatementSubmitClicked` → `ValidateActivePuzzle(true)`을 명시적으로 눌러야 판정한다(단, 이제는 실패해도 피드백이 뜬다 — 예전엔 진술서 실패 시 아무 표시도 없었다).

변경 필요 사항:
- [ ] `PhotoSlots`/`RequiredPhotoCount`를 실제로 요구하는 진술서 데이터가 아직 없어(Chapter 1은 전부 0), `WB_PuzzlePhotos`/`WB_PhotoSlots` 드래그 앤 드롭 경로는 PIE로 수동 검증만 했고 실 데이터 기준 검증은 못함 — 해당 데이터가 추가되면 재검증 필요.
- [ ] 본편 시나리오의 `LieText`/반증 조건이 [DataTable-Handoff.md](../DataTable-Handoff.md)에서 입력되는 대로 정상 매칭되는지 확인.
- [ ] 반증 실패/성공 피드백(`ShowPopup`)의 실제 문구·연출을 최종 확정(아트/이펙트/사운드 트랙과 연동).
- [ ] 인물별로 진술서가 여러 개일 때 진행 순서(잠금/해금)가 스토리 상태와 어긋나지 않는지 확인.

## 4. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: 위 "남은 수동 작업"(WBP_Tablet 재생성/수동 편집 + 아이콘 지정) 완료, 사진·진술서 본편 데이터 1차 연결
- **2단계(9/13~9/15, M2)**: 사진 읽음 표시 정책 반영, 반증 피드백 연출·사운드 연결
- **3단계(9/16~9/17, M3)**: 태블릿 내 다른 페이지(브라우저/메신저)와 전환 회귀 테스트, DT_Characters 행 추가 시 홈 화면 자동 반영 확인
- **4단계(9/18, M4)**: 최종 QA 대응

## 5. 완료 조건

- 홈 화면 인물 폴더가 `DT_Characters` 행 수·`FolderName`·`FolderSortOrder`만으로 결정되며, 코드에 인물별 하드코딩이 없다.
- 사진 분석 퍼즐과 진술서 반증이 본편 데이터 기준으로 동작하며 프로토타입 전용 하드코딩이 없다.
- 인물 폴더 전환(`ShowFolder`/`RefreshFolderContents`)이 4개 태블릿 기능(폴더/브라우저/메신저/홈)을 오가도 깨지지 않는다.
- 관련 자동화/스모크 테스트가 통과한다.

## 6. 주의 사항

- `WBP_Tablet`은 브라우저·메신저 담당자와 공유되는 UI이므로 동시 편집 전 확인한다.
- 이 페이지는 어떤 조사 상태도 자체적으로 저장하지 않는다. 항상 [Subsystem-Handoff.md](../Subsystem-Handoff.md)의 InvestigationSubsystem을 조회/갱신하는 방식으로 구현한다.
