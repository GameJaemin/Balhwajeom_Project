# WBP 및 C++ 런타임 UI 구조 정리

작성 기준: 2026-09-14 현재 프로젝트 구현

이 문서는 지금까지 제작·변경한 주요 Widget Blueprint와, 그 안에 C++이 실행 중 생성하는 UI가 어떤 데이터로 구성되고 어디에서 값이 변경되는지 정리한 문서다.

## 1. 가장 중요한 구조

현재 UI는 크게 두 층으로 나뉜다.

1. **WBP 디자이너 레이어**
   - 전체 배경, 패널, 버튼, 이미지 영역, `ScaleBox`, `SizeBox`, `CanvasPanel` 등 고정 레이아웃을 담당한다.
   - C++의 `BindWidgetOptional` 이름과 WBP 위젯 이름이 일치해야 한다.
2. **C++ 런타임 레이어**
   - 획득 키워드, 미획득 키워드 빈자리, 사진 타일, 폴더 섹션, 문장 빈칸, 증거사진 슬롯처럼 데이터 개수에 따라 달라지는 UI를 생성한다.
   - 데이터 테이블과 `UBalhwajeomInvestigationSubsystem`의 현재 상태를 읽어 WBP의 `WrapBox`, `ScrollBox`, `Overlay` 등에 자식 위젯을 넣는다.

```text
DataTable
  ├─ DT_Characters
  ├─ DT_Words
  ├─ DT_Photos
  └─ DT_Sentences
          │
          ▼
UBalhwajeomInvestigationSubsystem
  ├─ 획득 키워드
  ├─ 촬영 사진
  └─ 문장 해결 상태
          │
          ▼
UBalhwajeomTabletWidget / Camera HUD / IntroFlowActor
          │
          ├─ WBP에 텍스트·이미지·Visibility 설정
          └─ 키워드·타일·빈칸·슬롯을 런타임 생성
```

## 2. 주요 WBP 목록

| WBP | 에셋 경로 | 부모 C++ 클래스 | 역할 |
|---|---|---|---|
| `WBP_Tablet` | `/Game/Balhwajeom/UI/Tablet/WBP_Tablet` | `UBalhwajeomTabletWidget` | 태블릿 전체 화면, 홈/폴더/메신저/인터넷/메모 페이지 전환 |
| `WBP_TabletStatement` | `/Game/Balhwajeom/UI/Tablet/StateMent/WBP_TabletStatement` | `UBalhwajeomTabletDetailWidget` | 진술서 본문, 키워드, 증거사진 슬롯, 제출 UI |
| `WBP_TabletPhoto` | `/Game/Balhwajeom/UI/Tablet/WBP_TabletPhoto` | `UBalhwajeomTabletDetailWidget` | 촬영 사진 상세 보기와 분석문장 빈칸 UI |
| `WBP_TabletPersonFolder` | `/Game/Balhwajeom/UI/Tablet/WBP_TabletPersonFolder` | `UBalhwajeomTabletPersonFolderWidget` | 여동생/어머니/형 폴더 탭과 파일 목록 창 |
| `WBP_TabletFolderButton` | `/Game/Balhwajeom/UI/Tablet/WBP_TabletFolderButton` | `UBalhwajeomTabletFolderButton` | 태블릿 홈의 인물 폴더 버튼 한 개 |
| `WBP_TabletFileTile` | `/Game/Balhwajeom/UI/Tablet/WBP_TabletFileTile` | `UBalhwajeomTabletPhotoButton` | 진술서 또는 사진 파일 타일 한 개 |
| `WBP_TabletFolderSection` | `/Game/Balhwajeom/UI/Tablet/WBP_TabletFolderSection` | `UBalhwajeomTabletFolderSection` | `단서와 정보`, `증거 사진`, `추억 사진` 묶음 |
| `WBP_Messenger` 및 하위 WBP | `/Game/Balhwajeom/UI/Tablet/WBP_Messenger*` | Messenger 위젯 클래스들 | 방·메시지·키워드·날짜 구분선 |
| `WBP_Internet` 및 하위 WBP | `/Game/Balhwajeom/UI/Tablet/Internet/WBP_Internet*` | Internet 위젯 클래스들 | 탭 브라우저와 페이지별 키워드 |
| `WBP_CapturePhoto` | `/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto` | `UBalhwajeomCapturePhotoWidget` | 촬영 직후 잠시 표시되는 사진·문장·키워드 카드 |
| `WBP_PhotoWorldStory` | `/Game/Balhwajeom/UI/PhotoStory/WBP_PhotoWorldStory` | `UPhotoWorldStoryWidget` | 촬영 직후 월드에 표시되는 시간 기반 자막 |
| `WBP_MainMenu` | `/Game/Balhwajeom/UI/Title/WBP_MainMenu` | `UBalhwajeomMainMenuWidget` | 타이틀과 시작 버튼 |
| `WBP_ScreenFade` | `/Game/Balhwajeom/UI/Title/WBP_ScreenFade` | `UBalhwajeomScreenFadeWidget` | 전체 화면 검은 페이드 |
| `WBP_CinematicVideo` | `/Game/Balhwajeom/UI/Title/WBP_CinematicVideo` | `UBalhwajeomCinematicVideoWidget` | MediaTexture를 전체 화면에 표시 |

### 진술서 WBP 중복 경로 주의

프로젝트에는 다음 두 에셋이 모두 존재한다.

- 실제 런타임 사용: `/Game/Balhwajeom/UI/Tablet/StateMent/WBP_TabletStatement`
- 이전 위치에 남아 있는 에셋: `/Game/Balhwajeom/UI/Tablet/WBP_TabletStatement`

`UBalhwajeomTabletWidget`의 기본 `StatementDetailWidgetClass`는 **`StateMent` 폴더 안의 WBP**를 로드한다. 이전 위치의 에셋을 수정하면 게임 화면에 반영되지 않는다. 이름 변경 시 리다이렉터나 소프트 경로가 끊어지면 사진 WBP가 대신 연결되는 것처럼 보일 수 있으므로, 이름 변경 후에는 클래스 기본값과 소프트 경로를 함께 확인해야 한다.

## 3. WBP별 역할과 C++ 연결

## 3.1 `WBP_Tablet`

### WBP가 담당하는 것

- 태블릿 외형과 전체 화면 배치
- 홈, 인물 폴더, 메신저, 인터넷, 메모 페이지 컨테이너
- `WidgetSwitcher` 기반 페이지 전환
- 팝업 WBP가 들어가는 `PopupLayer`
- 홈의 인물 폴더가 들어가는 `WB_PersonFolders`
- `TabletUpAnim` 애니메이션

### C++가 담당하는 것

`UBalhwajeomTabletComponent`가 TAB 입력을 받고 `WBP_Tablet` 인스턴스를 생성한다.

- 열기: `RequestOpenTablet()` → `OpenTabletNow()`
- 진술서로 바로 열기: `RequestOpenTabletToStatement()`
- 닫기: `CloseTablet()`
- 열기 사운드: `/Game/Balhwajeom/Audio/SFX/TabletUp`
- 닫기 사운드: `/Game/Balhwajeom/Audio/SFX/TabletDown`
- 열기 애니메이션: `TabletUpAnim` 정방향
- 닫기 애니메이션: 같은 `TabletUpAnim` 역방향

`UBalhwajeomTabletWidget`는 다음을 실행 중 생성·갱신한다.

- `RefreshHomeFolders()`에서 `DT_Characters` 순서대로 홈 폴더 버튼 생성
- `ShowFolder()`에서 선택된 `CharacterID` 저장 후 폴더 페이지로 이동
- `ActivateDetailWidget()`에서 진술서 WBP 또는 사진 WBP를 `PopupLayer`에 동적으로 삽입
- 메신저·인터넷 버튼 이벤트 연결
- 읽지 않은 메시지 배지 갱신

### 홈 폴더 버튼 크기

홈 폴더는 런타임에서 각각 `USizeBox`로 감싼다.

- 크기: `109 × 100`
- 내용: `WBP_TabletFolderButton`
- 라벨: `DT_Characters.FolderName`
- 아이콘·라벨 폰트: `WBP_Tablet` 클래스 기본값의 `DefaultFolderIcon`, `FolderLabelFont`

이 크기를 WBP 자식에서만 늘려도 바깥 `SizeBox`가 제한할 수 있다. 전체 타일 크기를 바꾸려면 `RefreshHomeFolders()`의 런타임 크기도 함께 바꿔야 한다.

## 3.2 `WBP_TabletPersonFolder`

### WBP가 담당하는 것

- 폴더 창 배경
- 여동생/어머니/형 탭 이미지와 클릭 버튼
- 상단 폴더명 표시 영역
- 닫기 버튼
- 실제 폴더 섹션이 들어가는 `SB_EvidencePhotos`
- 스크롤 위치를 나타내는 `IMG_FolderScroll`
- `ScaleBox`와 `SizeBox`를 통한 비율 유지

주요 바인딩 이름은 다음과 같다.

- `TXT_FolderTitle`
- `TXT_FolderFeedback`
- `IMG_FolderTitleIcon`
- `BTN_FolderClose`
- `BTN_FolderSister`, `BTN_FolderMother`, `BTN_FolderBrother`
- 각 인물의 `Idle`, `Selected` 이미지
- `SB_EvidencePhotos`
- `IMG_FolderScroll`

### 폴더명 변경 과정

1. 탭 클릭 시 `OnTabRequested(TabIndex)`가 발생한다.
2. 부모 `UBalhwajeomTabletWidget`가 `ResolveFolderTabCharacterID()`로 인물을 찾는다.
3. `RefreshFolderContents()`가 선택 인물의 `FCharacterDefinition`을 읽는다.
4. `SetFolderHeader(FolderName, Icon)`이 `TXT_FolderTitle`과 아이콘을 변경한다.

즉, 화면의 `여동생`, `어머니`, `형` 폴더명은 고정 이미지가 아니라 `DT_Characters.FolderName`을 통해 바뀐다.

### 폴더 섹션 분류 규칙

`RefreshFolderContents()`는 촬영된 사진만 가져와 다음처럼 분류한다.

| 영역 | 조건 |
|---|---|
| 단서와 정보 | 고정 진술서 또는 `PhotoSentenceID`가 있지만 아직 분석문장을 해결하지 않은 사진 |
| 증거 사진 | `PhotoSentenceID`가 있고 해당 분석문장을 해결한 사진 |
| 추억 사진 | `PhotoSentenceID`가 없는 일반 사진 |

분석문장이 필요한 사진만 미완성/완성 상태로 분류되며, 분석문장이 없는 사진은 추억 사진으로 들어간다.

### 파일 타일 이름과 툴팁

`WBP_TabletFileTile`에 전달되는 이름은 다음 값이다.

- 진술서: `"{FolderName} 진술서"`
- 사진: `DT_Photos.PhotoName`

라벨은 제한된 폭에서 말줄임표로 표시된다. 타일 또는 파일명 위에 마우스를 올리면 전체 이름을 확인할 수 있도록 툴팁이 설정된다. 타일 바깥의 런타임 `SizeBox` 크기는 `152 × 125`이며 `ClipToBounds`가 적용된다.

### 스크롤바

- 실제 스크롤: `SB_EvidencePhotos`
- 별도 표시 이미지: `IMG_FolderScroll`
- `HandleFolderScrolled()`가 현재 오프셋을 읽어 표시 위치를 갱신한다.

기본 Slate 스크롤바의 두께와 별도 `IMG_FolderScroll`의 폭은 서로 다른 값이다. 화면에 보이는 막대가 무엇인지 확인한 뒤 수정해야 한다.

## 3.3 `WBP_TabletFolderButton`

WBP에는 기본적으로 다음 이름이 연결된다.

- `BTN_Folder`
- `IMG_FolderIcon`
- `TXT_FolderLabel`

실행 중 `Configure(CharacterID, Label, IconTexture, LabelFont)`가 값을 넣는다. 따라서 텍스트 내용을 WBP 디자이너에서 바꿔도 게임 시작 후 `DT_Characters.FolderName`으로 덮어쓴다.

클릭하면 `OnFolderSelected(CharacterID)`를 부모 태블릿에 전달한다.

## 3.4 `WBP_TabletFileTile`

주요 바인딩은 다음과 같다.

- `BTN_File`
- `SB_Thumbnail`
- `IMG_Thumbnail`
- `TXT_Label`

실행 중 `Configure(PhotoID, Label, Thumbnail)`가 사진과 이름을 설정한다. 썸네일이 없는 진술서 타일도 같은 WBP를 재사용한다.

클릭하면 `OnPhotoSelected(PhotoID)`가 발생한다. 진술서 타일에서는 `PhotoID` 자리에 `SentenceID`가 전달되며, 연결된 핸들러에 따라 진술서 또는 사진 상세 화면을 연다.

## 3.5 `WBP_TabletFolderSection`

폴더의 각 묶음을 표현한다.

- `HeaderButton`: 접기/펼치기
- `ArrowText`: 펼침 상태 화살표
- `TitleText`: `단서와 정보`, `증거 사진`, `추억 사진`
- `ContentWrapBox`: `WBP_TabletFileTile`들이 들어가는 영역

`Configure(Title, bStartExpanded)`와 `AddTile()`이 실행 중 제목과 개수를 변경한다. 제목에는 현재 타일 수가 함께 표시된다.

## 3.6 `WBP_TabletStatement`

### 실제 사용하는 에셋

`/Game/Balhwajeom/UI/Tablet/StateMent/WBP_TabletStatement`

### WBP가 담당하는 것

- 기준 화면 크기 `1274 × 907`
- 진술서 배경 이미지
- 진술서 제목·본문·반증서 영역
- 고정 삽화와 증거사진 영역
- 우측 키워드 목록 영역
- 키워드 개수 텍스트
- 찾아보기, 닫기, 제출 서명하기 버튼
- 사진 선택용 `WBP_TabletPersonFolder`
- `ScaleBox → SizeBox → Canvas` 기반 레이아웃

### 중요한 클래스 기본값

`UBalhwajeomTabletDetailWidget`의 Class Defaults에서 다음을 직접 바꿀 수 있다.

| 속성 | 사용 위치 |
|---|---|
| `KeywordFont` | 우측 키워드와 키워드 드래그 글자 |
| `KeywordFontSize` | 우측 키워드와 드래그 글자 크기 |
| `StatementTextFont` | 진술서 본문, 문장 조각, 진술서 빈칸의 글자 |
| `StatementTextFontSize` | 위 텍스트의 크기 |
| `SelectedPhotoResultFont` | 선택한 증거사진 아래 `ResultText` |
| `SelectedPhotoResultFontSize` | 선택한 증거사진 결과 문장 크기 |

`BindActiveDetailWidgets()`가 상세 WBP 생성 직후 이 값을 다시 읽어 동적 텍스트에 적용한다. 특히 `TXT_SelectedPhotoResult`는 C++에서 왼쪽 정렬로 설정한다.

### 키워드 목록 생성

`RefreshAcquiredWordsDisplay()` 또는 `RefreshPuzzleControls()`가 `DT_Words` 전체 순서를 기준으로 목록을 만든다.

- 획득함: `UBalhwajeomTabletWordChip` 생성
- 미획득: 같은 위치에 `USpacer(106 × 39)` 생성
- 결과: 획득 여부와 관계없이 `DT_Words`의 칸 순서가 유지됨
- `WB_PuzzleWords`가 두 열짜리 `WrapBox`가 되도록 WBP의 폭과 슬롯 크기를 맞춤
- 개수: `획득한 수 / DT_Words 전체 행 수`

따라서 키워드의 표시 이름과 순서는 WBP가 아니라 `DT_Words`에서 바뀐다.

### 키워드 호버와 드래그

동적 키워드는 `UBalhwajeomTabletWordChip`가 만든다.

- 호버 텍스처: `/Game/Balhwajeom/UI/Tablet/StateMent/keyword_hover`
- 표시 크기: `106 × 39`
- 호버 시 글자색: 검정
- 평상시 진술서 키워드 배경: 투명
- 드래그 기준점: `EDragPivot::CenterCenter`
- 드래그 배경색: sRGB `RGB(255, 237, 217)`
- 글자 정렬: 가운데
- 드래그 폰트: 원래 키워드의 폰트와 크기를 복사

호버는 태블릿 부모 위젯의 색조 영향을 받을 수 있지만 드래그 데코레이터는 별도 Slate 레이어에 표시된다. 그래서 두 곳을 같은 색으로 보이게 하려면 드래그 쪽 색상을 C++에서 명시적으로 지정해야 한다.

### 진술서 문장과 증거사진

1. `HandleStatementTileSelected()`가 폴더 진술서를 연다.
2. `PreparePuzzle(SentenceID)`가 제출 상태를 초기화한다.
3. `BuildSentenceBuilder()`가 `SentenceTemplate`의 `[]`를 기준으로 문장을 나눈다.
4. 일반 문구는 `UTextBlock`, 빈칸은 `UBalhwajeomTabletSentenceBlank`로 생성한다.
5. `BuildPhotoSlots()`가 `FSentencePhotoSlot` 수만큼 증거사진 슬롯을 만든다.
6. 모든 키워드와 사진이 채워져도 진술서는 자동 제출하지 않고 `제출 서명하기` 버튼을 눌러야 검증한다.

증거사진 슬롯을 클릭하면 `OpenPhotoPicker()`가 같은 개인 폴더 UI를 연다.

- 분석 완료 사진: 선택 가능
- 분석 미완료 사진: 목록에 표시하지만 반투명이고 선택 불가
- 미완료 사진 클릭 메시지: `아직은 증거로 사용할 수 없을 것 같다.`
- 이미 넣은 사진도 슬롯을 다시 클릭하여 교체 가능

선택한 사진의 분석 결과는 `DT_Sentences.ResultText`에서 가져와 사진 영역 아래 `TXT_SelectedPhotoResult`에 표시한다.

### 제출 버튼 호버

제출 버튼은 호버 시 버튼 자체의 크기나 위치를 바꾸지 않는다. 색상·투명도 변화만 사용해야 한다. WBP 애니메이션이나 버튼 Style에 `Render Scale`, `Pressed Padding`이 추가되면 다시 작아지거나 왼쪽 위로 움직일 수 있다.

## 3.7 `WBP_TabletPhoto`

### WBP가 담당하는 것

- 사진 이름
- 촬영된 이미지
- 사진 분석문장 영역
- 우측 키워드 목록
- 닫기 버튼
- `ScaleBox`와 `SizeBox` 기반 화면 비율 유지

### 실행 흐름

1. 폴더 사진 타일 클릭 → `OpenPhoto(PhotoID)`
2. `DT_Photos`에서 `PhotoSentenceID` 확인
3. 분석문장이 없으면 `CustomDescription`을 일반 문장으로 표시
4. 분석문장이 있고 미해결이면 빈칸 퍼즐을 생성
5. 분석문장을 해결했으면 `DT_Sentences.ResultText`를 표시

사진의 상단 제목은 우선 촬영 당시 `ObjectID`로 `DT_EvidenceDefinitions.ObjectName`을 찾는다. 이름을 찾지 못하면 `DT_Photos.PhotoName`을 사용한다.

### 분석문장 생성

`BuildSentenceBuilder()`는 진술서와 같은 코드를 사용하지만 `SentenceType == PhotoAnalysis`인 경우 사진 UI 스타일을 적용한다.

- 문장 글자 크기: C++ 기본 `27`
- 빈칸 최소 크기: `83 × 36`
- 키워드를 넣으면 빈칸이 단어 폭에 맞게 가로로 늘어남
- 빈칸의 단어는 주변 문구와 같은 크기 사용
- 오답 시 사각형 배경은 표시하지 않고 문구·빈칸 텍스트만 `#C27878` 계열로 변경
- 정답 시 퍼즐을 숨기고 `ResultText` 표시

정답 결과 문장의 가로 폭은 주로 WBP의 결과 텍스트를 감싸는 `SizeBox`/`CanvasPanelSlot` 폭과 `Auto Wrap Text` 설정으로 결정된다.

## 3.8 `WBP_CapturePhoto`

촬영 직후 잠시 표시되는 카드다.

### WBP 구조

```text
SB_ViewportScale (ScaleToFit)
└─ SB_Design1920x1080
   └─ ViewportRoot
      ├─ ScreenDimmer
      ├─ CardRoot (928 × 721, 고정 배치 컨테이너)
      │  ├─ CardComposite (RetainerBox, 카드 통합 애니메이션 대상)
      │  │  └─ CardVisualRoot
      │  │     ├─ CardBackground
      │  │     ├─ CapturedPhotoImage
      │  │     └─ SentenceBackground
      │  │        └─ SentenceOverlay
      │  │           ├─ SentenceTextBlock
      │  │           └─ SentenceBuilder
      │  └─ KeywordList (키워드별 순차 애니메이션 대상)
      └─ TabFlyTarget (기존 호환용, 현재 런타임에서는 사용하지 않음)
```

`CardComposite`는 배경·사진·문장을 한 장의 렌더 결과로 합성한다. 진입 이동·회전과 퇴장 페이드는
중심 피벗 `(0.5, 0.5)`을 사용하는 이 위젯 하나에만 적용한다. 따라서 내부 요소가 같은 축을 공유하고,
페이드 중 겹친 영역이 각각 비치는 현상을 막는다. 에펙의 스케일 값은 사용하지 않아 크기는 항상 유지된다.

애니메이션은 에펙의 60fps 기준 37~110프레임을 잘라 총 73프레임(약 1.217초)으로 재현한다.
카드는 오른쪽에서 90도 회전된 상태로 들어와 20프레임에 자리 잡는다. 키워드는 첫 카드보다 5프레임 늦게
시작하며, 추가 키워드도 5프레임 간격으로 같은 동작을 반복한다. 53프레임부터 `CardRoot`가 아래로 이동하므로
카드와 모든 키워드가 함께 움직이고, 63~73프레임에는 카드 합성 결과와 각 키워드가 사라진다.
퇴장 위치는 에펙 시간 베지어의 앞 키프레임 `Speed 0 / Influence 90%`, 뒤 키프레임
`Speed 0 / Influence 0%`를 사용한다. 이 베지어는 위치에만 적용하며 63~73프레임의 투명도는 선형이다.
`ScreenDimmer`는 이동하지 않고 63~73프레임에 투명도만 낮아진다. 촬영 순간의 기존 HUD 흰색 플래시는
그대로 사용하며, 에펙의 `그냥 배경`과 별도 촬영 이펙트는 재현하지 않는다.

### 분석문장/자연어 분기

촬영 완료 시 `UBalhwajeomPhotoCameraComponent`가 `PhotoDefinition.PhotoSentenceID`를 조회한다.

- 연결 문장의 `SentenceType == PhotoAnalysis`: 분석문장 카드
- 그 외 또는 `PhotoSentenceID` 없음: 자연어 카드

| 종류 | 배경 | 사진 색조 | 문장 |
|---|---|---|---|
| 분석문장 | `photo_black_bg_v2` | 원본 색조 | 흰 문구와 `83 × 36` 흰 빈칸 |
| 자연어 | `photo_yellow_bg_v2` | `#E0D8C8` | 검은색 일반 문장 |

`photo_black_IMG`, `photo_yellow_IMG`는 디자인 가이드이며 런타임 배경으로 사용하지 않는다.

### 촬영 카드에서 C++이 생성하는 것

`PresentCapture()`가 다음을 실행 중 만든다.

- 분석문장: `SentenceTemplate`을 `[]`로 분리하여 `SentenceBuilder`에 문구와 흰 빈칸 추가
- 새로 획득한 키워드: `KeywordList`에 `UBorder + UTextBlock`을 하나씩 추가
- 사진: 촬영된 PNG를 `CapturedPhotoImage`의 Brush로 설정
- 배경과 사진 Tint: 분석문장 여부에 따라 교체

### 시간과 애니메이션을 바꾸는 곳

`WBP_CapturePhoto`의 Class Defaults → `Capture Photo` 카테고리에서 변경한다.

| 속성 | 의미 | 기본값 |
|---|---|---:|
| `Photo Info Display Duration` | 전체 크기로 읽을 수 있게 유지하는 시간 | `1.0s` |
| `Photo Fly To Tab Duration` | 사진이 TAB으로 이동하는 시간 | `0.45s` |
| `Keyword Follow Delay` | 사진 도착 후 키워드 이동 시작까지 지연 | `0.08s` |
| `Keyword Fly To Tab Duration` | 키워드 이동 시간 | `0.32s` |
| `TabTargetScale` | 도착 시 사진 축소 비율 | `0.12` |
| `KeywordFontAsset`, `KeywordFontSize` | 촬영 카드 키워드 폰트 | 크기 `25` |
| `AnalysisSentenceFontSize` | 분석문장 미리보기 글자 크기 | `24` |
| `NaturalSentenceFontSize` | 자연어 미리보기 글자 크기 | `25` |

사진과 키워드가 TAB에 도착한 뒤 `FinishCapturePhotoPresentation()`이 HUD 키워드 카운트를 갱신한다. 실제 획득 데이터는 촬영 등록 시 이미 저장되지만, 숫자만 애니메이션 종료 시점까지 이전 값으로 보류한다.

## 3.9 게임 화면의 키워드 카운터

TAB 버튼 위의 `15/32` 표시는 별도 WBP가 아니라 `UBalhwajeomKeywordCounterWidget`가 C++로 전체 트리를 생성한다.

- 루트: `KeywordCounterCanvas`
- 텍스트: `TXT_GameplayKeywordCount`
- 위치: 우측 하단 Anchor, `Position(-5, -115)`
- 크기: `180 × 34`
- 값: `GetAcquiredWords().Num() / GetAllWordDefinitions().Num()`

위치를 직접 바꾸려면 `BalhwajeomKeywordCounterWidget.cpp`의 `CountSlot->SetPosition()`을 수정한다. WBP 디자이너에서는 찾을 수 없다.

일반 상호작용으로 획득한 키워드는 `OnWordAcquired` 이벤트에서 즉시 갱신한다. 사진 촬영으로 얻은 키워드는 촬영 카드 이동이 끝날 때까지 갱신을 보류한다.

## 3.10 타이틀·페이드·시네마틱 WBP

### `WBP_MainMenu`

- 필수 버튼 이름: `BTN_Start`
- 클릭하면 `OnStartRequested` 발생
- 중복 클릭 방지를 위해 첫 클릭 후 버튼을 비활성화
- 전체 화면 크기는 WBP의 `ScaleBox/SizeBox`와 Anchor가 담당

### `WBP_ScreenFade`

- 검은 전체 화면 위젯
- C++이 `RenderOpacity`를 시간에 따라 변경
- `FadeToBlack(Duration)`와 `FadeFromBlack(Duration)` 사용
- 완료 시 각각 Delegate를 발생시켜 다음 상태로 넘어감

### `WBP_CinematicVideo`

- 필수 이미지 이름: `IMG_Video`
- `SetMediaTexture()`가 `MediaTexture`를 Brush Resource로 넣음
- Brush 기준 크기 `1920 × 1080`

### `BP_IntroFlowController`와의 관계

`ABalhwajeomIntroFlowActor`가 세 WBP를 생성하고 상태를 관리한다.

```text
타이틀
  → 시작 클릭
  → 페이드 아웃
  → 인트로 MP4 또는 IntroSequence
  → 페이드 전환
  → 게임 시작
  → 설정 시 태블릿 진술서 자동 열기
  → 진술서 완료 후 태블릿 닫기
  → 엔딩 페이드/영상
  → 타이틀 복귀
```

- MediaSource + MediaPlayer + MediaTexture가 모두 있으면 MP4가 `IntroSequence`보다 우선한다.
- 셋 중 하나라도 없으면 `IntroSequence`를 사용한다.
- Level Sequence가 화면에 보이려면 Camera Cuts Track과 올바른 카메라 바인딩이 필요하다.
- `bOpenStatementAfterIntro`가 켜져 있으면 게임 시작 시 첫 인물 폴더의 첫 진술서를 연다.
- 엔딩 시작 시 BGM을 Fade Out하고, 엔딩 미디어가 없으면 `EndingSequence`를 사용한다.

## 3.11 메신저 WBP 묶음

메신저는 하나의 WBP가 아니라 다음 하위 WBP를 런타임에 조합한다.

| WBP | 부모 C++ 클래스 | 역할 |
|---|---|---|
| `WBP_Messenger` | `UBalhwajeomMessengerWidget` | 방 목록과 현재 대화 화면 전체 |
| `WBP_MessengerRoom` | `UBalhwajeomMessengerRoomWidget` | 대화방 목록 한 줄 |
| `WBP_MessengerMessage` | `UBalhwajeomMessengerMessageWidget` | 메시지 말풍선 한 개 |
| `WBP_MessengerKeyword` | `UBalhwajeomMessengerKeywordWidget` | 메시지 속 획득 가능 키워드 |
| `WBP_MessengerDateSeparator` | `UBalhwajeomMessengerDateSeparator` | 날짜가 바뀌는 지점의 날짜 구분선 |

`WBP_Messenger`는 `DA_MessengerCatalog`를 읽고 방마다 `WBP_MessengerRoom`을 생성한다. 방을 선택하면 해당 Room Data Asset의 메시지 순서대로 날짜 구분선과 `WBP_MessengerMessage`를 생성한다.

메시지 본문·발신자·시각·플레이어 메시지 여부·키워드 연결은 WBP의 고정 텍스트가 아니라 Messenger Room Data Asset에서 변경한다. 메시지 키워드를 선택하면 연결된 `WordID`가 조사 시스템에 등록되어 태블릿과 게임 HUD의 키워드 목록에 반영된다.

주요 데이터 경로:

- `/Game/Balhwajeom/Data/Messenger/DA_MessengerCatalog`
- `/Game/Balhwajeom/Data/Messenger/Rooms/DA_MessengerRoom_*`

## 3.12 인터넷 WBP 묶음

인터넷도 메인 WBP가 탭과 페이지 WBP를 동적으로 조합한다.

| WBP | 부모 C++ 클래스 | 역할 |
|---|---|---|
| `WBP_Internet` | `UBalhwajeomInternetWidget` | 인터넷 창, 탭 목록, 페이지 전환 |
| `WBP_InternetTab` | `UBalhwajeomInternetTabWidget` | 브라우저 탭 한 개 |
| `WBP_InternetKeyword` | `UBalhwajeomInternetKeywordWidget` | 웹페이지의 클릭 가능한 키워드 |
| `WBP_InternetPage_Main` | `UBalhwajeomInternetPageWidget` | 메인 페이지 |
| `WBP_InternetPage_Weather` | `UBalhwajeomInternetPageWidget` | 날씨 페이지 |
| `WBP_InternetPage_News1` | `UBalhwajeomInternetPageWidget` | 뉴스 페이지 1 |
| `WBP_InternetPage_News2` | `UBalhwajeomInternetPageWidget` | 뉴스 페이지 2 |
| `WBP_InternetPage_Ad` | `UBalhwajeomInternetPageWidget` | 광고 페이지 |

`UBalhwajeomInternetWidget`가 페이지 클래스와 탭 클래스를 로드하고 현재 탭·방문 기록·앞으로/뒤로 이동 상태를 관리한다. 각 페이지의 `WBP_InternetKeyword`는 `SetupKeyword()`로 `DT_Words`의 `WordID`와 연결된다. 따라서 인터넷 키워드의 실제 획득 상태는 다른 키워드와 동일하게 `UBalhwajeomInvestigationSubsystem`에 저장된다.

페이지 배치와 고정 문구는 각 페이지 WBP에서 수정하고, 페이지 전환 규칙·탭 생성·키워드 획득 처리는 `BalhwajeomInternetWidget.cpp`, `BalhwajeomInternetPageWidget.cpp`, `BalhwajeomInternetKeywordWidget.cpp`에서 수정한다.

## 3.13 사진 월드 스토리 WBP

`/Game/Balhwajeom/UI/PhotoStory/WBP_PhotoWorldStory`는 `UPhotoWorldStoryWidget`을 부모로 사용한다. 사진 촬영 직후 월드에 표시되는 시간 기반 자막에 사용되며, 태블릿 사진 상세 화면의 `ResultText`와는 별도다.

- 표시 텍스트 위젯: `StoryText`
- 데이터: `DT_Photos.WorldStoryCues` 또는 레거시 `WorldStoryLines`
- 문장 효과음: `DT_Photos.StoryCueSound`. 각 문장이 나타날 때마다 처음부터 재생되며, 비어 있거나 로드에 실패해도 자막은 계속 진행된다. `StoryVoice`는 태블릿·침대 회상용 음성으로 유지된다.
- 자막 전환 시점: 각 `FPhotoStoryCue.StartTimeSeconds`
- 마지막 문장 유지 시간: `DT_Photos.LastCueDurationSeconds`. 시간이 지나면 월드 스토리의 기본 페이드 아웃이 시작된다.

이 자막은 촬영 장면 연출용이므로 태블릿에서 사진을 다시 열 때 반복 표시하지 않는다.

## 4. 데이터가 UI 값을 바꾸는 방식

## 4.1 DataTable 연결 위치

`Config/DefaultGame.ini`에서 조사 시스템의 테이블 경로를 지정한다.

| 설정 | 기본 에셋 |
|---|---|
| `EvidenceDefinitionsTable` | `DT_EvidenceDefinitions` |
| `EvidenceStatesTable` | `DT_EvidenceStates` |
| `WordsTable` | `DT_Words` |
| `PhotosTable` | `DT_Photos` |
| `KeywordDocumentsTable` | `DT_KeywordDocuments` |
| `KeywordChoicesTable` | `DT_KeywordChoices` |
| `SentencesTable` | `DT_Sentences` |
| `CharactersTable` | `DT_Characters` |

## 4.2 `DT_Characters`

주요 필드:

- `CharacterID`: 다른 테이블과 연결하는 내부 ID
- `FolderName`: 화면에 표시되는 폴더명
- `FolderSortOrder`: 홈 폴더 순서

인물 이름이나 폴더 순서를 바꿀 때 수정한다.

## 4.3 `DT_Words`

주요 필드:

- `WordID`: 문장 정답과 연결하는 내부 ID
- `DisplayWord`: 화면에 표시되는 키워드
- `RelatedCharacterIDs`: 어느 인물 폴더와 연결되는지
- `bUnlockedByDefault`: 처음부터 획득한 상태인지

키워드 칸의 순서와 전체 개수는 `GetAllWordDefinitions()` 결과, 즉 테이블 행 정렬을 따른다.

## 4.4 `DT_Photos`

주요 필드:

- `PhotoID`: 사진 내부 ID
- `PhotoName`: 폴더 타일 이름
- `CustomDescription`: 분석문장이 없는 사진의 자연어 설명
- `PhotoSentenceID`: 사진 자체의 분석문장
- `CharacterID`: 들어갈 인물 폴더
- `EvidenceSentenceID`: 진술서에 잘못 제출했을 때 표시할 설명 문장
- `GrantedWordIDs`: 사진 촬영 시 획득하는 키워드
- `StoryVoice`, `WorldStoryCues`: 촬영 직후 월드 연출

`PhotoSentenceID` 유무가 폴더의 추억 사진 여부와 촬영 카드 테마를 결정한다.

## 4.5 `DT_Sentences`

주요 필드:

- `SentenceID`
- `SentenceType`: `Statement` 또는 `PhotoAnalysis`
- `CharacterID`
- `bIsFolderStatement`: 개인 폴더에 고정 진술서로 노출할지
- `SentenceTemplate`: `[]`를 포함한 미완성 문장
- `WordSlots`: 각 빈칸의 정답 `WordID`
- `PhotoSlots`: 증거사진 정답 `PhotoID`
- `RequiredPhotoCount`
- `ResultText`: 정답 후 표시 문장

`SentenceTemplate`의 `[]` 개수와 `WordSlots` 개수/인덱스가 맞아야 한다.

## 4.6 조사 상태 저장

`UBalhwajeomInvestigationSubsystem`은 다음 런타임 상태를 보관한다.

- `FAcquiredWordRecord`: 획득 키워드와 획득 출처
- `FCapturedPhotoRecord`: 촬영 사진 ID, 오브젝트 ID, 저장 경로, 촬영 시각
- 해결된 문장과 제출 상태

촬영 PNG는 콘텐츠 에셋이 아니라 `Saved` 아래의 상대 경로로 저장된다. 태블릿은 `GetOrLoadCapturedPhotoTexture()`에서 파일을 읽어 `UTexture2D`로 만들고 캐시한다.

## 5. 어떤 값을 어디에서 수정해야 하는가

| 바꾸고 싶은 항목 | 수정 위치 | 주의점 |
|---|---|---|
| 고정 패널 위치·크기 | 해당 WBP Designer의 Canvas Slot | 부모 `SizeBox` 제한 확인 |
| 전체 화면 대응 | WBP의 `ScaleBox`, 디자인 `SizeBox` | Anchor만 바꾸면 종횡비가 깨질 수 있음 |
| 진술서 키워드 폰트 | `WBP_TabletStatement` Class Defaults | `KeywordFont`, `KeywordFontSize` |
| 진술서 본문/빈칸 폰트 | 같은 Class Defaults | `StatementTextFont`, `StatementTextFontSize` |
| 선택 사진 결과 폰트 | 같은 Class Defaults | `SelectedPhotoResultFont`, Size |
| 사진 분석문장 크기 | `BalhwajeomTabletWidget.cpp` | 현재 C++이 `27`로 지정 |
| 키워드 이름·순서·전체 수 | `DT_Words` | WBP 텍스트를 바꿔도 런타임에 덮어씀 |
| 폴더명·순서 | `DT_Characters` | 탭 이름 매칭도 확인 |
| 사진 파일명 | `DT_Photos.PhotoName` | 타일은 런타임에 이 값으로 갱신 |
| 자연어 사진 설명 | `DT_Photos.CustomDescription` | `PhotoSentenceID`가 있으면 분석문장이 우선 |
| 사진 분석문장·정답 | `DT_Sentences` + `DT_Photos.PhotoSentenceID` | `[]`와 WordSlots 일치 필요 |
| 진술서 정답 사진 | `DT_Sentences.PhotoSlots` | 촬영·분석 완료된 사진만 정상 선택 가능 |
| 키워드 호버/드래그 색 | `BalhwajeomTabletWidget.cpp` | 동적 위젯이라 WBP에 없음 |
| 폴더 타일 런타임 크기 | `MakeFolderTileSlot()` | 현재 `152 × 125` |
| 게임 HUD 키워드 숫자 위치 | `BalhwajeomKeywordCounterWidget.cpp` | 순수 C++ 위젯 |
| 촬영 카드 시간·폰트·Tint | `WBP_CapturePhoto` Class Defaults | `Capture Photo` 카테고리 |
| 촬영 카드 고정 배치 | `WBP_CapturePhoto` Designer | 바인딩 이름 유지 |
| 타이틀 시작 버튼 | `WBP_MainMenu` | 이름 `BTN_Start` 유지 |
| 인트로/엔딩 미디어 | 맵의 `BP_IntroFlowController` Details | Media 3종이 모두 있어야 MP4 사용 |
| 태블릿 열기/닫기 사운드 | `UBalhwajeomTabletComponent` 기본값 | `TabletUp`, `TabletDown` |

## 6. WBP에서 바꿔도 C++이 덮어쓰는 값

다음 값은 실행 시 C++이 다시 설정하므로 WBP의 미리보기 값만 바꾸면 게임에 반영되지 않을 수 있다.

- 폴더명과 사진 파일명
- 촬영된 사진 Brush
- 진술서·사진의 본문과 결과 문장
- 키워드 목록과 개수
- 키워드 드래그 시 배경·폰트·정렬
- 동적 문장 빈칸 크기·색상
- 증거사진 슬롯의 썸네일과 이름
- `TXT_SelectedPhotoResult`의 폰트 및 왼쪽 정렬
- `WBP_CapturePhoto`의 테마 배경, 사진 Tint, 문장 색상·크기
- 페이드 위젯의 Render Opacity
- 시네마틱 위젯의 MediaTexture Brush

반대로 다음은 WBP Designer에서 바꾸는 것이 적절하다.

- 고정 배경 이미지와 장식
- Canvas Slot 위치·크기
- Anchor와 Alignment
- `ScaleBox`/`SizeBox` 구조
- 버튼의 기본/호버/눌림 Style
- 동적 자식이 들어갈 `WrapBox`, `ScrollBox`, `Overlay`의 위치와 폭

## 7. 위젯 이름 변경 시 주의사항

C++의 `BindWidgetOptional`은 이름으로 연결된다. 아래와 같은 위젯 이름을 변경하면 기능 연결이 사라질 수 있다.

- `WB_PuzzleWords`
- `WB_SentenceBuilder`
- `WB_PhotoSlots`
- `TXT_PuzzleKeywordCount`
- `TXT_SelectedPhotoResult`
- `WBP_PhotoPickerFolder`
- `SB_EvidencePhotos`
- `CapturedPhotoImage`
- `SentenceTextBlock`
- `SentenceBuilder`
- `KeywordList`
- `CardRoot`, `CardComposite`, `CardBackground`, `SentenceBackground`
- `BTN_Start`
- `IMG_Video`

에셋 이름을 변경할 때는 Unreal의 Rename 기능으로 리다이렉터를 생성하고, 이후 소프트 경로 문자열도 검색해야 한다. 파일 탐색기에서 `.uasset` 이름만 바꾸면 안 된다.

## 8. 에디터 생성 함수 사용 시 주의사항

에디터 모듈의 핵심 파일:

- `Source/BalhwajeomEditor/Private/Tablet/TabletWidgetBlueprintLibrary.cpp`

이 파일에는 WBP를 생성하거나 다시 구성하는 함수가 있다.

- `CreateTabletWidgetBlueprint()`
- `RedesignTabletWidgetBlueprint()`
- `CreateTabletDesignerWidgets()`
- `RedesignTabletStatementWidget()`
- `RedesignTabletPhotoWidget()`
- `InstallTabletPersonFolderWidget()`
- `CreateCapturePhotoWidgetBlueprint()`
- `CreateIntroFlowAssets()`

`Redesign...` 또는 현재 `CreateCapturePhotoWidgetBlueprint()`처럼 기존 WidgetTree를 지우고 다시 만드는 함수는 WBP Designer에서 직접 조정한 값을 덮어쓸 수 있다. 수동 조정 후에는 해당 재생성 함수를 무심코 다시 실행하지 않는 것이 안전하다.

## 9. 주요 소스 파일

| 파일 | 역할 |
|---|---|
| `Source/Balhwajeom/Public/Tablet/BalhwajeomTabletWidget.h` | 태블릿 WBP 계약, BindWidget, 동적 위젯 클래스 선언 |
| `Source/Balhwajeom/Private/Tablet/BalhwajeomTabletWidget.cpp` | 폴더·진술서·사진·키워드·드래그 UI 실행 로직 |
| `Source/Balhwajeom/Public/Tablet/BalhwajeomTabletComponent.h` | TAB 입력, 열기/닫기 인터페이스 |
| `Source/Balhwajeom/Private/Tablet/BalhwajeomTabletComponent.cpp` | 태블릿 생성, 입력 차단, 사운드, 애니메이션 |
| `Source/Balhwajeom/CameraSystem/BalhwajeomCapturePhotoWidget.h/.cpp` | 촬영 직후 카드의 테마·문장·키워드·이동 애니메이션 |
| `Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceCameraHUD.cpp` | 촬영 카드 표시 시간과 종료 시 HUD 카운트 갱신 |
| `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp` | 촬영 데이터 저장, 분석문장 판별, 키워드 전달 |
| `Source/Balhwajeom/Private/UI/BalhwajeomKeywordCounterWidget.cpp` | TAB 위의 키워드 현재/전체 개수 UI |
| `Source/Balhwajeom/Private/Intro/BalhwajeomIntroFlowActor.cpp` | 타이틀·인트로·게임·엔딩 상태 전환 |
| `Source/BalhwajeomEditor/Private/Tablet/TabletWidgetBlueprintLibrary.cpp` | WBP 생성·재구성 도구 |

## 10. 수정 후 확인 순서

1. 수정한 WBP가 실제 런타임 경로의 에셋인지 확인한다.
2. `BindWidgetOptional` 위젯 이름을 유지했는지 확인한다.
3. WBP Compile과 Save를 한다.
4. 데이터 내용 변경이면 `DT_Words`, `DT_Photos`, `DT_Sentences`, `DT_Characters`의 내부 ID 연결을 확인한다.
5. 플레이에서 다음을 순서대로 확인한다.
   - 태블릿 열기/닫기
   - 폴더 탭과 섹션 분류
   - 파일명 말줄임과 툴팁
   - 사진 분석문장 정답/오답
   - 진술서 사진 선택·교체·제출
   - 촬영 카드의 검정/노랑 테마 분기
   - 촬영 카드가 TAB에 도착한 뒤 키워드 숫자 갱신
   - 인트로 종료 후 진술서 자동 열기
   - 최종 진술서 완료 후 엔딩 및 타이틀 복귀
