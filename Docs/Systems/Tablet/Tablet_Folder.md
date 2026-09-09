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
- [ ] `WBP_Tablet` Class Defaults에서 `Default Folder Icon`에 공용 폴더 아이콘 텍스처를 지정(홈 화면 폴더 버튼과 폴더 창 타이틀 아이콘에 함께 쓰인다).
- [x] `TABLET_SMOKE` 자동화(`RunTabletWidgetSmokeTest`) 재실행 통과 확인(2026-09-09). `Balhwajeom.Investigation` 자동화는 별도로 재실행 필요.
- [ ] 본편 인물 구성이 확정되면 `DT_Characters` CSV에 실제 인물 행(현재는 `SISTER` 1건)과 `FolderSortOrder`를 채운다 — [DataTable-Handoff.md](../DataTable-Handoff.md) 담당자와 진행.
- [ ] `Default Folder Icon`을 아직 지정하지 않았다면 `IMG_FolderTitleIcon`/홈 화면 폴더 버튼 모두 아이콘 없이(텍스트만) 보인다 — 아트 통합 트랙에서 처리.

## 2. 사진 / 진술서 폴더 타일 (완료, 2026-09-09)

`ShowFolder(CharacterID)`가 페이지를 연 뒤 `RefreshFolderContents`가 두 곳을 채운다:
- `WB_EvidencePhotos`(WrapBox): 촬영된 사진 타일들만(진술서는 더 이상 여기 없음). 각 타일은 ~170x170(`SizeBox`)로 홈 화면 폴더 아이콘과 거의 같은 크기다. `UBalhwajeomTabletWidget::GetOrLoadCapturedPhotoTexture(PhotoID)`가 `FImageUtils::ImportFileAsTexture2D`로 촬영된 PNG(`FCapturedPhotoRecord::ImageRelativePath`)를 런타임에 디코드해 실제 썸네일로 보여준다(디코드 결과는 `CapturedPhotoTextureCache`에 캐시).
- `SB_StatementTile`(SizeBox, 170x170 고정): 폴더 창 **하단 중앙**에 단독으로 배치된 진술서 타일(있는 경우만 표시, 없으면 `Collapsed`). 사진과 같은 위젯(`UBalhwajeomTabletPhotoButton`)을 재사용하지만 썸네일 없이 텍스트만(`"{FolderName} 진술서"`) 표시하고 `HandleStatementTileSelected(SentenceID)`로 연결된다 — 인물별 진술서는 현재 1개까지만 지원(`VisibleStatementIDs[0]`).

두 타일 종류 모두 라벨은 `SetAutoWrapText(false)` + `SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis)`로, 실제 윈도우 탐색기처럼 한 줄로 표시하고 넘치면 "..."으로 잘리게 했다(썸네일이 생기면서 줄바꿈 텍스트가 타일 밖으로 잘려 보이던 문제 해결).

사진을 클릭하면 `OpenPhoto`가 `Photo.PhotoSentenceID` 존재 여부로 화면을 분기한다(둘 다 팝업 이미지(`IMG_PopupPhoto`)에 같은 캐시된 텍스처를 표시):
- **분석 문장 있음**(`SentenceType=PhotoAnalysis`): 문장 빈칸(`SentenceTemplate`/해결 시 `ResultText`)과 함께 `PreparePuzzle`이 키워드 후보 목록(`WB_PuzzleWords`)을 인터랙티브하게 띄운다.
- **분석 문장 없음**(자연어만) 또는 이미 해결된 문장: `CustomDescription`/`WorldStoryLines`/`ResultText`를 그대로 보여주고, 퍼즐 컨트롤(단어 슬롯 채우기 등)은 띄우지 않는다.

`ShowPopup(Title, Body, PhotoTexture = nullptr)`가 팝업의 단일 진입점이다. `PhotoTexture`가 없으면(예: 진술서 팝업) `IMG_PopupPhoto`를 `Collapsed` 처리한다.

**획득 키워드 표시 위치 변경(완료, 2026-09-09)**: 예전에는 폴더 페이지 자체에 `TXT_AcquiredWords`/`WB_AcquiredWords`가 상시 노출됐지만, 지금은 폴더 페이지에서 완전히 제거하고 **사진 또는 진술서를 열었을 때만** 팝업 안에서 보이도록 옮겼다. `ShowPopup`이 항상 `RefreshAcquiredWordsDisplay()`를 호출해 팝업의 `WB_PuzzleWords`에 그 인물의 획득 키워드 전체를 표시한다(해결 여부·문장 유무와 무관하게 항상 표시). 이후 `PreparePuzzle`→`RefreshPuzzleControls`가 실행되면(분석 문장이 있고 미해결일 때) 같은 `WB_PuzzleWords`를 퍼즐용 인터랙티브 후보 목록으로 덮어써 대체한다. 즉 `WB_PuzzleWords`는 이제 "정보 표시"와 "퍼즐 후보 선택" 두 역할을 겸한다. `EPISODE 01`/`EPISODE 02` 구분 텍스트는 요청대로 완전히 제거했다.

**폴더 창 UI(완료, 2026-09-09)**: 실제 윈도우 탐색기 창처럼 보이도록 폴더 페이지 상단을 "타이틀바" 구조로 바꿨다 — 사용자 확인 사항: 색상은 태블릿 기존 세피아톤 유지(구조만 윈도우식), 주소창/검색창은 만들지 않음, 아이콘 배치는 기존 그리드(큰 아이콘) 뷰 유지.
- 왼쪽: `IMG_FolderTitleIcon`(작은 폴더 아이콘, `DefaultFolderIcon` 공유) + `TXT_FolderTitle`.
- 오른쪽: `BTN_FolderClose`("×" 버튼, 실제로는 기존 `HandleBackClicked`/`NavigateBack()`을 그대로 호출 — 어차피 PersonFolder에서 갈 수 있는 이전 페이지는 Home뿐이라 "닫기"와 "뒤로가기"가 동일하다).
- `TXT_FolderSubtitle`("보관된 기록")은 윈도우 타이틀바에 없는 요소라 제거했다. 주소창/검색창은 만들지 않았다(사용자 확인).
- 기존 `BTN_FolderBack`은 `BTN_FolderClose`로 이름을 바꿨다(`BTN_PopupClose`와 네이밍 통일).

변경 필요 사항:
- [ ] 사진 퍼즐(`AvailablePuzzleWordIDs`/`AvailablePuzzlePhotoIDs`, `NextWordSlotCursor`/`NextPhotoSlotCursor`)의 슬롯 수가 본편 문장 길이와 맞는지 실제 데이터로 검증.
- [ ] 사진 갤러리의 읽음/안읽음 표시(`bViewedInTablet`, 로드맵 P1-2)를 이 페이지에 반영할지 확정.
- [ ] 인물별 진술서가 2개 이상 필요해지면 `VisibleStatementIDs[0]` 고정 대신 여러 타일을 만들도록 확장한다.

## 3. 진술서 (Statement / 진술 반증)

현재 상태: 완료. 위 폴더 그리드의 진술서 타일 클릭 → `HandleStatementTileSelected(SentenceID)` → `ActiveSentenceID`+`ActiveSubmission`(`FSentenceSubmission`)에 선택한 키워드·완성 사진을 채운 뒤 `BTN_StatementSubmit`/`HandleStatementSubmitClicked` → `ValidateActivePuzzle(true)` → `InvestigationSubsystem::ValidateSentence`로 반증한다. 해결된 추리 사진만 `GetStatementSentencesForCharacter`를 통해 후보로 노출된다. (기존에는 `BTN_EvidenceStatement`라는 별도 고정 버튼이었으나, 2026-09-09에 폴더 그리드의 동적 타일로 통합했다.)

변경 필요 사항:
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
