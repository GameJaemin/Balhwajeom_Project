# 태블릿 담당 작업 문서 — 인물 폴더 (사진 · 진술서)

> 기준 문서: [Project-Progress-and-Roadmap.md](../../Project-Progress-and-Roadmap.md) 2.3(태블릿과 추리), 5장 P0-2/P1-2
> 담당 범위: `UBalhwajeomTabletWidget`의 `PersonFolder` 페이지 — 사진/인물 폴더 조회, 사진 분석 퍼즐, 진술서(진술 반증)
> 같은 태블릿 폴더의 다른 문서: [태블릿_브라우져.md](./태블릿_브라우져.md), [태블릿_메신져.md](./태블릿_메신져.md)

핵심 파일: [BalhwajeomTabletWidget.h](../../../Source/Balhwajeom/Public/Tablet/BalhwajeomTabletWidget.h) / `.cpp`, `WBP_Tablet.uasset`(`Content/Balhwajeom/UI/Tablet`)

`UBalhwajeomTabletWidget`은 태블릿의 5개 페이지(`ETabletPage`: `Home`, `PersonFolder`, `Messenger`, `Internet`, `Memo`)를 한 위젯에서 전환하는 공용 클래스다. 이 헤더는 [태블릿_브라우져.md](./태블릿_브라우져.md), [태블릿_메신져.md](./태블릿_메신져.md) 담당자와 공유되므로, `UBalhwajeomTabletWidget.h`를 직접 수정할 때는 다른 두 담당자에게 미리 공지한다.

## 1. 인물 폴더 배치 (홈 화면)

현재 상태: **완료(2026-09-09)**. 기존에는 `EFamilyMember`(Sister/Brother/Mother) 고정 enum과 `BTN_Sister`/`BTN_Brother`/`BTN_Mother` 3개 고정 버튼으로 홈 화면을 구성했으나, `DT_Characters` 기반 완전 동적 생성으로 교체했다.

- `UBalhwajeomInvestigationSubsystem::GetAllCharacterDefinitions`가 `DT_Characters`의 모든 행을 `FolderSortOrder`(동률이면 `CharacterID` 사전순)로 정렬해 반환한다.
- `UBalhwajeomTabletWidget::RefreshHomeFolders()`가 `NativeOnInitialized` 시 이 목록으로 홈 화면의 `WB_PersonFolders`(WrapBox)를 채운다. 각 항목은 새 위젯 `UBalhwajeomTabletFolderButton`(공용 폴더 아이콘 + `FolderName` 텍스트)이며, 클릭 시 `HandleHomeFolderSelected` → `ShowFolder(CharacterID)`로 인물 폴더 페이지를 연다.
- `ActiveCharacterID`는 이제 enum이 아니라 `FName`(DT_Characters의 CharacterID)이다. `GetActiveCharacterID()`가 public BlueprintPure로 노출된다.
- 폴더 버튼 아이콘은 인물별 전용 아트가 아니라 **공용 폴더 아이콘 1개**를 쓰기로 결정함(사용자 확인 완료). `UBalhwajeomTabletWidget::DefaultFolderIcon`(EditDefaultsOnly)에 텍스처를 지정한다.
- `WBP_Tablet` 생성 스크립트([TabletWidgetBlueprintLibrary.cpp](../../../Source/BalhwajeomEditor/Private/Tablet/TabletWidgetBlueprintLibrary.cpp)의 `BuildHomePage()`)도 3개 고정 아이콘 버튼 대신 `WB_PersonFolders` WrapBox 하나만 배치하도록 수정했다.

인물이 늘어나면 **코드 수정 없이 `DT_Characters`에 행만 추가**하면 홈 화면 폴더가 자동으로 늘어난다.

### 남은 수동 작업 (에디터에서 진행 필요)

- [ ] C++ 재컴파일 후 `WBP_Tablet`을 재생성(`BuildWidgetBlueprint`/redesign 스크립트 실행) 또는 기존 `WBP_Tablet`에서 `BTN_Sister`/`BTN_Brother`/`BTN_Mother`를 수동 삭제하고 `WB_PersonFolders`(WrapBox)를 같은 위치에 추가.
- [ ] `WBP_Tablet` Class Defaults에서 `Default Folder Icon`에 공용 폴더 아이콘 텍스처를 지정.
- [ ] `TABLET_SMOKE` 자동화(`RunTabletWidgetSmokeTest`)와 `Balhwajeom.Investigation` 자동화를 재실행해 통과 확인.
- [ ] 본편 인물 구성이 확정되면 `DT_Characters` CSV에 실제 인물 행(현재는 `SISTER` 1건)과 `FolderSortOrder`를 채운다 — [DataTable-Handoff.md](../DataTable-Handoff.md) 담당자와 진행.

## 2. 사진 / 진술서 퍼즐

`ShowFolder(CharacterID)`가 페이지를 연 뒤 `RefreshFolderContents`가 `InvestigationSubsystem::GetPhotosForCharacter`/`GetAcquiredWordsForCharacter`로 사진·키워드 목록(`WB_EvidencePhotos`, `WB_AcquiredWords`)을 채운다. `OpenPhoto`→`PreparePuzzle`로 사진 분석 퍼즐(정답 슬롯 채우면 자동 판정, `ValidateSentence`)을 실행한다.

변경 필요 사항:
- [ ] 사진 퍼즐(`AvailablePuzzleWordIDs`/`AvailablePuzzlePhotoIDs`, `NextWordSlotCursor`/`NextPhotoSlotCursor`)의 슬롯 수가 본편 문장 길이와 맞는지 실제 데이터로 검증.
- [ ] 사진 갤러리의 읽음/안읽음 표시(`bViewedInTablet`, 로드맵 P1-2)를 이 페이지에 반영할지 확정.

## 3. 진술서 (Statement / 진술 반증)

현재 상태: 프로토타입 완료. `BTN_EvidenceStatement`/`HandleStatementClicked`로 진입, `ActiveSentenceID`+`ActiveSubmission`(`FSentenceSubmission`)에 선택한 키워드·완성 사진을 채운 뒤 `BTN_StatementSubmit`/`HandleStatementSubmitClicked` → `ValidateActivePuzzle(true)` → `InvestigationSubsystem::ValidateSentence`로 반증한다. 해결된 추리 사진만 `GetStatementSentencesForCharacter`를 통해 후보로 노출된다.

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
