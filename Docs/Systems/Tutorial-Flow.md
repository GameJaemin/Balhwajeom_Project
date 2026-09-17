# 튜토리얼 / 단계 진행 시스템

> 설계 기록: [2026-09-13-room2-tutorial-design.md](../superpowers/specs/2026-09-13-room2-tutorial-design.md)
> **상태: 구현 완료. room3에서 바로 플레이 가능.**
> 적용 레벨: `/Game/Levels/room3`

---

## 1. 바로 해보기

1. 에디터에서 `/Game/Levels/room3` 를 연다
2. **Play**
3. 아래 순서대로 진행된다

| 단계 | 화면 | 가능한 입력 |
|---|---|---|
| **DustTeach** | 먼지 쌓인 사진을 **바라보면** 화면이 어두워지고 `[F]` 프롬프트가 **깜빡인다** | **F만** (우클릭·TAB 잠김) |
| **PhotoPrompt** | **사진 한 장만 털면 바로** 화면 전체 암전 + **카메라 아이콘이 깜빡인다** | **우클릭 해금** |
| **Photograph** | 1인칭 카메라. 암전은 사라지고 **`WBP_CAM` 뷰파인더**가 덮인다 | 우클릭으로 나와 F, 다시 우클릭으로 촬영 |
| **CompleteFamilyPhoto** | 사진 세 장을 모두 찍으면 태블릿 잠금 해제 + **태블릿 아이콘 깜빡임** | TAB을 열고 `가족 사진`의 빈칸 문장 완성 |
| **Done** | — | 문이 `[F] 문 열기`로 바뀐다 |

> **연출이 두 겹이다.** 설명은 전부 전체화면 오버레이(2.5절)가 맡고,
> 디렉터는 **"지금 이걸 눌러라"** 를 아이콘 깜빡임으로만 가리킨다.
> 각 깜빡임은 자기 설명 오버레이를 읽은 뒤에야 시작한다 — 아래 표의 `Hint Required Tags`.
> **화면 암전은 전 단계에서 껐다.** 오버레이가 이미 화면을 덮으며 어둡게 하는데
> 그 아래에서 상시 암전까지 걸리니 방이 계속 어두워 보였다.

> **화면에 설명 문구는 띄우지 않는다.** 안내는 두 가지뿐이다 —
> 화면을 어둡게 해서 쓸 것만 남기고, **지금 눌러야 할 것을 밝기로 깜빡인다.**
> 깜빡임 대상은 `Hint Target` 이 정한다: `InteractPrompt`(= `WBP_Interact` 의 `[F]`),
> `PhotoCameraIcon` / `TabletIcon`(= `WBP_HUID` 의 아이콘).

사진 세 장은 벽에 걸린 `Evidence_OBJ_01_001/002/003`,
문은 `room3`의 `GateDoor_Exit` 이다. 촬영 후 액자의 가족 대화는 선택 콘텐츠로 남지만
튜토리얼 완료나 문 해금 조건에는 포함되지 않는다.

### 자동 검증

```bash
"D:\HDD_UnrealEngine\UE_5.7\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\UnrealProjects\Balhwajeom\Balhwajeom.uproject" -ExecCmds="Automation RunTests Balhwajeom.Tutorial" -TestExit="Automation Test Queue Empty" -unattended -nopause -nosplash -NullRHI -DisablePlugins=Fab
```

그중 `Balhwajeom.Tutorial.Room2.FlowEndToEnd` 는
**실제 `DA_TutorialFlow_Room2` 와 실제 DataTable** 로 전체 흐름을 끝까지 돌려본다 —
태그 오타, 빠진 `PostCaptureStateID`, ini 미등록 태그는 레벨이 아니라 이 테스트에서 먼저 깨진다.
`Balhwajeom.Tutorial.Room3.GateConfiguration` 은 실제 `room3.umap`의 문이
`Evidence.SentenceSolved.PHOTO_01_003` 하나만 요구하는지도 검사한다.

---

## 2. 구조

```
[3] UBalhwajeomTutorialFocusWidget (ZOrder 5)   암전 + 아이콘 하이라이트
        ▲ pure 함수 2개만 읽는다. 태그도 맵도 모른다.
[2] ABalhwajeomTutorialDirector + UBalhwajeomTutorialFlow
        ▲ Runtime.Lock.* / Evidence.* 태그로만 통신
[1] PhotoCameraComponent / TabletComponent / DoorInteractionComponent
        잠금 태그가 "있으면" 막는다. 튜토리얼의 존재를 모른다.
```

**핵심 규칙: 잠금 태그가 없으면 항상 열려 있다.**
디렉터를 놓지 않은 레벨(Level_Main 포함)은 **설정 0개로 기존과 똑같이 동작한다.**

### 뷰포트 레이어 (암전이 ZOrder 5인 이유)

| ZOrder | 위젯 | 암전 아래/위 |
|---|---|---|
| 0 | `WBP_HUID` — 카메라/태블릿 아이콘 | 아래 (어두워짐) |
| 1 | `WBP_HUD2` — 베드 메모리 | 아래 |
| **5** | **`UBalhwajeomTutorialFocusWidget`** | **암전 + 하이라이트** |
| 10 | `WBP_Interact` — 상호작용 프롬프트 | **위 (밝게 남음)** |
| 50 | `WBP_CAM` — 카메라 뷰파인더 | 위 |
| 100 / 250 | 태블릿·포커스가이드 / 촬영 카드 | 위 |

`[F]` 프롬프트는 `ShouldShowInteractionPrompt()` 가 **바라보고 있고 + 가깝고 + 상호작용 가능할 때만** 띄운다.
그래서 "바라볼 때만 강조"가 레이어 순서만으로 해결되고, 추가 코드가 필요 없다.

하이라이트 아이콘은 `WBP_HUID` 의 아이콘(`Image_Camera` / `Image_TAB`)에서
**브러시와 화면 사각형을 그대로 복사**한다. HUD 레이아웃을 바꿔도 자동으로 따라가므로
아이콘을 손으로 복제할 필요가 없다. 이름 후보를 배열로 받으므로(`Image_Camera` → `Image_0` 순)
HUD 이미지 이름이 바뀌어도 하이라이트가 조용히 사라지지 않는다.

### 깜빡임

`RenderOpacity` 를 코사인으로 흔든다. 스텝이 시작되는 순간이 가장 밝은 지점이라 놓치지 않는다.

- **HUD 아이콘**(`PhotoCameraIcon` / `TabletIcon`): 밝은 복사본의 투명도를 흔든다.
  0 이면 암전 아래의 원래 아이콘이 비쳐 어둡게, 1 이면 완전히 밝게 보인다.
- **`[F]` 프롬프트**(`InteractPrompt`): `WBP_Interact` 는 ZOrder 10 이라 위에 덮을 수 없으므로,
  PlayerController 가 프롬프트 페이드에 깜빡임을 **곱해서** 그린다.

**시계는 디렉터가 가진다.** 두 프레젠터가 각자 시간을 재면 서로 어긋나므로,
`ABalhwajeomTutorialDirector::GetTutorialHighlightPulse()` 하나를 같이 읽는다.
파라미터도 디렉터 액터에 있다.

| 프로퍼티 | 기본값 | 뜻 |
|---|---|---|
| `Highlight Pulses Per Second` | 0.9 | 초당 밝음→어두움→밝음 왕복 횟수 |
| `Highlight Pulse Min Opacity` | 0.0 | 가장 어두울 때 |
| `Highlight Pulse Max Opacity` | 1.0 | 가장 밝을 때 |

> ⚠️ **`[F]` 깜빡임은 암전에 영향을 주면 안 된다.** `FollowInteractPrompt` 암전은
> 프롬프트 알파에 비례하는데, 거기에 깜빡임까지 섞이면 **화면 전체가 같이 깜빡인다.**
> 그래서 `GetInteractionPromptAlpha()` 는 **깜빡임이 적용되지 않은** 원본 페이드 값을 돌려주고,
> 깜빡임은 위젯에 그려지는 값에만 곱한다.

### 카메라 뷰파인더

우클릭으로 카메라를 들면 `ABalhwajeomEvidenceCameraHUD` 가
**`/Game/Balhwajeom/UI/HUD/WBP_CAM`** 을 ZOrder 50 에 띄운다(`Viewfinder Widget Class`).
포커스 가이드(100)·촬영 카드(250) 보다 아래라 그 둘은 프레임 위에 계속 보인다.
촬영 순간에는 `SetCaptureUIHiddenForScreenshot` 이 꺼서 **저장되는 사진에는 안 들어간다.**
클래스를 비우면 예전처럼 중앙 십자선만 그린다.

---

## 2.4 디렉터 연출과 오버레이의 경계

`DA_TutorialFlow_Room2` 는 **7단계**다. 앞의 두 단계는 인트로 태블릿을 기다리는 용도라
화면에 아무것도 그리지 않는다.

| # | StepID | Hint Target | Hint Required Tags | 끝나는 조건 |
|---|---|---|---|---|
| 0 | (이름 없음) | — | — | 태블릿 모드 진입 |
| 1 | `TabletIntro` | — | — | 탐색 모드 복귀 (5초 자동) |
| 2 | `DustTeach` | `[F]` 프롬프트 | `Tutorial.Overlay.Seen.Interaction` | 액자 하나 먼지털기 |
| 3 | `PhotoPrompt` | 카메라 아이콘 | `Tutorial.Overlay.Seen.PhotoCamera` | 카메라 모드 진입 |
| 4 | `Photograph` | `[F]` 프롬프트 | `Tutorial.Overlay.Seen.MemoryObject` | 사진 3장 |
| 5 | `CompleteFamilyPhoto` | 태블릿 아이콘 | `Tutorial.Overlay.Seen.Tablet` | 가족 사진 문장 완성 |
| 6 | `Done` | — | — | — |

**모든 단계의 `Dim Mode` 는 `Off`** 다. 암전 기능 자체는 남아 있으니 필요하면 데이터에서
다시 켤 수 있다.

`HintRequiredTags` 는 **단계의 진행이 아니라 표현만** 막는다. 단계는 평소처럼 돌고
완료 조건도 그대로 평가되며, 아이콘 깜빡임과 암전만 태그가 찰 때까지 나오지 않는다.
태그는 매 프레임 다시 읽으므로, 조건이 풀리면 하이라이트도 다시 사라진다.

`TabletIntro` 는 원래 게임을 켜자마자 태블릿 아이콘을 깜빡이며 화면을 어둡게 했다.
플레이어가 아직 아무 설명도 못 들은 시점이라 표현을 통째로 껐고, 단계 자체는
인트로 태블릿을 기다리는 타이밍 역할로 남겼다.

### 사라질 때 페이드

하이라이트는 **꺼질 때 페이드 아웃한다.** 예전에는 단계가 끝나는 순간 툭 사라져서,
우클릭으로 카메라를 드는 순간 아이콘이 깜빡이던 밝기 그대로 증발했다.

| 대상 | 페이드 주체 |
|---|---|
| HUD 아이콘 하이라이트 | `UBalhwajeomTutorialFocusWidget::HighlightInterpolationSpeed` |
| `[F]` 프롬프트 깜빡임 | `ABalhwajeomCameraPlayerController::InteractionPromptPulseBlend` |

프롬프트 쪽은 깜빡임을 끄는 게 아니라 **깜빡임의 영향력을 0으로 줄인다.**
그냥 껐다면 마지막 밝기에서 멈춰 프롬프트가 한 번 번쩍이며 사라졌을 것이다.

### 설정 스크립트

```text
Scripts/Tutorial/ConfigureTutorialHintGates.py
```

멱등이라 다른 브랜치에서 플로우를 머지한 뒤 다시 돌리면 된다.
`Balhwajeom.Tutorial.HintGateConfiguration` 이 7단계 전부를 검사한다.

---

## 2.5 전체화면 설명 오버레이

암전·깜빡임과 **별개 시스템**이다. 디렉터가 "지금 뭘 누를지"를 밝기로 알려준다면,
오버레이는 화면 전체를 덮고 **글로 설명한 뒤 아무 키나 눌러야 넘어간다.**

| 에셋 | 역할 |
|---|---|
| `DT_TutorialOverlay` | 8개 화면. 조건 · 이미지 · 제목 · 설명 · 완료 태그 |
| `WBP_TutorialOverlay` | 딤 + 블러 위에 이미지 / 제목 / 설명 / `아무 키나 눌러 계속` |
| `UBalhwajeomTutorialOverlayPresenter` | 플레이어 컨트롤러에 붙는 컴포넌트. 언제 어떤 행을 띄울지 |

### 언제 뜨는가

`RequiredTags` 규칙은 하나다 — **이전 행의 `Tutorial.Overlay.Seen` 태그 + 자기 트리거.**
이전 행을 조건에 넣는 이유는 순서 때문이다. 카메라 모드에서 나오지 않고 액자 3장을
연달아 찍으면 `Evidence.Photographed.*` 3개가 3인칭 복귀보다 **먼저** 붙어서,
이전 행 조건이 없으면 태블릿 안내가 회상 안내보다 앞서 뜬다.

| 행 | 내용 | 트리거 |
|---|---|---|
| `OVL_01_001` | 진술서 | `Tutorial.Trigger.GameplayStarted` |
| `OVL_01_002` | 이동과 시점 | `Tutorial.Trigger.TabletClosed` |
| `OVL_01_003` | 상호작용 | `Tutorial.Trigger.InteractPromptShown` |
| `OVL_01_004` | 카메라 | `Tutorial.Trigger.InteractCompleted` |
| `OVL_01_005` | 회상 | `Tutorial.Trigger.PhotoCaptureCompleted` |
| `OVL_01_006` | 태블릿 | `Evidence.Photographed.OBJ_01_001~003` |
| `OVL_01_007` | 여동생 폴더 | `Tutorial.Trigger.TabletOpened` |
| `OVL_01_008` | 사진 추리 | `Tutorial.Trigger.SisterFolderOpened` |

`Tutorial.Trigger.*` 는 `StoryStateTags` 의 **네이티브 태그라 ini 등록이 필요 없다.**
C++ 만 붙이는 태그라 ini 와 철자가 어긋날 자리를 만들지 않았다.

`TabletOpened` / `TabletClosed` / `SisterFolderOpened` 는 **상태 태그**다. 나머지는 1회성.
인트로가 `bOpenStatementAfterIntro` 로 태블릿을 **이미 한 번 열기 때문에**, 1회성이면
007의 조건이 게임 시작 시점에 충족돼서 006을 닫자마자 엉뚱한 곳에서 떠버린다.

001은 그 인트로 태블릿 **위에** 뜬다(진술서 설명이니 그게 맞다). 002는 그 태블릿을
닫아야 뜬다 — `TabletClosed` 를 조건에 넣은 이유다. 안 그러면 이동 설명이 태블릿 위에 뜬다.

### 태그를 붙이는 곳

| 태그 | 위치 |
|---|---|
| `GameplayStarted` | `ABalhwajeomIntroFlowActor::HandleFadeFromBlackFinished()` |
| `InteractPromptShown` | `ABalhwajeomCameraPlayerController::UpdateInteractionPrompt()` |
| `InteractCompleted` | `ABalhwajeomCameraPlayerController::CloseInteractionModal()` |
| `PhotoCaptureCompleted` | `UBalhwajeomPhotoCameraComponent` 카메라 모드 이탈, 촬영 1장 이상일 때만 |
| `TabletOpened` / `TabletClosed` | `UBalhwajeomTabletComponent::OpenTabletNow()` / `FinishCloseTablet()` — 항상 둘 중 하나만 |
| `SisterFolderOpened` | `UBalhwajeomTabletWidget::SetTabletPage()` |

인트로 없이 레벨을 바로 PIE 로 켜면 `GameplayStarted` 를 붙일 액터가 없다.
제시자가 첫 틱에 `ABalhwajeomIntroFlowActor` 가 없으면 **스스로 붙인다.**

### 표시 방식

```text
조건 성립 → 입력 차단(즉시) → 0.5초 빈 화면 → 페이드 인 0.35초
                                                  → 2초 입력 잠금 → 고정문구 등장 + 깜빡임
                                                  → 아무 키 → 페이드 아웃 0.25초
                                                                          ↓
                                                        CompletionTag 기록 → 다음 큐 항목
```

**입력 차단과 화면 등장은 시점이 다르다.** 차단은 조건이 성립한 그 프레임에 걸리고
(그래야 그 순간의 입력이 게임에 닿지 않는다), 화면은 `ShowDelaySeconds` 만큼 늦게
나타나기 시작한다. 없으면 우클릭한 바로 그 프레임에 설명이 얼굴 앞에 붙어서,
게임이 반응하는 게 아니라 끼어드는 것처럼 읽힌다.

2초 입력 잠금은 **빈 화면 0.5초가 끝난 뒤부터** 센다. 그래야 플레이어가 기다리는
2초가 실제로 읽을 수 있는 2초가 된다.

- ZOrder **2000**. 상호작용 모달(1300)보다 위라 **태블릿 위에도 뜬다**
- 잠금은 Slate `IInputProcessor` 로 건다. 뷰포트에 닿기 전에 삼키므로 **뒤에 열려 있는
  태블릿 버튼도 눌리지 않는다.** 촬영 결과 카드와 같은 방식이고, "아무 키" 판정도
  `BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey` 를 그대로 쓴다
- 잠금 중에는 `아무 키나 눌러 계속` 을 **숨긴다.** 못 누르는 동안 깜빡이면 거짓말이다
- **키를 떼는 이벤트는 삼키지 않는다.** 삼키면 게임은 그 키가 계속 눌려 있다고 믿는다 —
  W 를 누른 채 오버레이가 뜨고 뒤에서 손을 떼면, 오버레이가 닫히는 순간 혼자 걸어간다
- 열 때와 닫을 때 `FlushPressedKeys()` 를 부른다. 들어올 때는 붙잡고 있던 키를 놓은 것으로
  정리하고, 나갈 때는 **오버레이를 닫은 그 키가 게임 입력으로 새어 들어가지 않게** 한다
- 한 번에 하나만. 한 태그로 두 행이 동시에 자격을 얻으면 Row 순서대로 큐에 쌓인다

### 이미지 추가

`Scripts/Investigation/README.md` 참조. 원본은 저장소에 올리지 않고
`Scripts/Tutorial/OverlayImages/` 에 두고 `ImportOverlayImage.py` 로
`/Game/Balhwajeom/UI/Tutorial/Overlay` 에 임포트한 뒤, 생성 스크립트의 `ROWS` 에 적는다.

### 알려진 한계

`Tutorial.Overlay.Seen.*` 는 `UStoryStateSubsystem`(GameInstance) 에만 있다.
**게임을 껐다 켜면 오버레이가 다시 나온다.** SaveGame 확장 시 함께 저장해야 한다.

---

## 3. 구현된 것

### C++

| 위치 | 내용 |
|---|---|
| `StoryStateTags` | 네이티브 태그 `Runtime.Lock` / `.PhotoCamera` / `.Tablet`, `Tutorial.Trigger.*` 6개 — **ini 등록 불필요** |
| `UStoryStateSubsystem` | 상태 전이 시 `Evidence.State.<StateID>`, 사진 문장 완성 시 `Evidence.SentenceSolved.<PhotoID>`, 월드 스토리 재생 시 `Evidence.StoryPlayed.<StateID>` 와 `Evidence.StoryHeard.<ObjectID>` 발행 |
| `UBalhwajeomPhotoCameraComponent` | `BlockedByTags` 잠금. 진입만 차단, **이탈은 항상 허용** |
| `UBalhwajeomTabletComponent` | `BlockedByTags` 잠금 (`ToggleTablet` / `RequestOpenTablet` 양쪽) |
| `UDoorInteractionComponent` | `UnlockRequiresTags` / `UnlockQuery`, `IsOpen()` |
| `ABalhwajeomGateDoorActor` | 잠금 문 + 월드 라벨. `[열 수 없는 문]` ↔ `[F] 문 열기` |
| `ABalhwajeomCameraPlayerController` | ZOrder 5 레이어 생성, `GetInteractionPromptAlpha()`, `[F]` 프롬프트 깜빡임 |
| `ABalhwajeomEvidenceCameraHUD` | `WBP_CAM` 뷰파인더 위젯 (ZOrder 50) |
| `UBalhwajeomTutorialFocusWidget` | 암전 + **깜빡이는 아이콘 하이라이트**. 트리를 C++에서 만들므로 위젯 BP 불필요 |
| `UBalhwajeomTutorialFlow` | 레벨당 1개 만드는 데이터 에셋 |
| `ABalhwajeomTutorialDirector` | 플로우 실행기. `IsHintAllowed()` 로 표현만 게이팅 |
| `ABalhwajeomEvidenceActor` | 월드 스토리 재생 시 StoryPlayed 태그 발행 |
| `UBalhwajeomTutorialOverlayPresenter` | `DT_TutorialOverlay` 를 읽어 오버레이를 띄우는 컨트롤러 컴포넌트 |
| `BalhwajeomTutorialOverlayQueue` | 어떤 행이 지금 떠야 하는지 판정하는 순수 함수 |
| `FTutorialOverlayInputProcessor` | 오버레이가 떠 있는 동안 모든 입력을 Slate 단계에서 삼킴 |

### 데이터

| 에셋 | 변경 |
|---|---|
| `DA_TutorialFlow_Room2` | 스텝 5개. 마지막 조건은 `가족 사진` 분석 문장 완성 |
| `DT_EvidenceStates` | `STATE_01_002_MEMORY` / `STATE_01_003_MEMORY` **행 추가**<br>`STATE_01_002_CLEAR` / `_003_CLEAR` 에 `PostCaptureStateID` 연결<br>`STATE_01_001_CLEAR/MEMORY` 의 플레이스홀더 메쉬 정리 |
| `DefaultGameplayTags.ini` | `Tutorial.Stage.*`, `Evidence.State.STATE_01_00X_CLEAR/MEMORY`, `Evidence.StoryPlayed.STATE_01_00X_MEMORY` |
| `room3.umap` | `DA_TutorialFlow_Room2` 디렉터 배치<br>`GateDoor_Exit`의 해금 조건을 `Evidence.SentenceSolved.PHOTO_01_003`으로 설정 |

> 삼남매 사진 세 장(`OBJ_01_001~003`)과 가족대화 자막(`DT_Photos.WorldStoryCues`)은
> **이미 프로젝트에 있던 데이터**다. 새로 만들지 않고 그대로 썼고,
> 002/003 에만 빠져 있던 촬영 후 상태를 001 과 같은 모양으로 채웠다.
> `PHOTO_01_003`은 `SENT_01_PHOTO_001`과 연결되어 있으며, 세 사진이 지급하는
> `WORD_01_001~003`으로 문장을 완성한다.

---

## 4. 다른 레벨에 적용하는 법

**코드는 한 줄도 건드리지 않는다.** 4단계다.

### 1) 태그 등록 — `Config/DefaultGameplayTags.ini`

```ini
+GameplayTagList=(Tag="Tutorial.Stage.<스텝이름>",DevComment="")
+GameplayTagList=(Tag="Evidence.State.<StateID>",DevComment="")
+GameplayTagList=(Tag="Evidence.Photographed.<ObjectID>",DevComment="")
+GameplayTagList=(Tag="Evidence.StoryPlayed.<StateID>",DevComment="")
+GameplayTagList=(Tag="Evidence.StoryHeard.<ObjectID>",DevComment="")
+GameplayTagList=(Tag="Evidence.SentenceSolved.<PhotoID>",DevComment="")
```

`<StateID>` / `<ObjectID>` 는 DataTable 의 Row Name 과 **글자 그대로 같아야** 한다.
`Runtime.Lock.*` 은 네이티브 태그라 등록이 필요 없다.

> ⚠️ **가장 흔한 버그**: 등록하지 않은 태그는 `RequestGameplayTag(..., ErrorIfNotFound=false)` 가
> **조용히 무시한다.** 에러도 로그도 없다. "조건이 절대 만족되지 않는다" 증상이면 여기부터 본다.

### 2) DataTable 행 추가

`DT_EvidenceDefinitions` / `DT_EvidenceStates` / `DT_Photos`.
증거 하나의 표준 모양은 `OBJ_01_001` 을 그대로 따라간다:

| StateID | Behavior | Presentation | Next / PostCapture | bCanCapture | StateMesh |
|---|---|---|---|---|---|
| `..._DUST` | `ChangeState` | `SimpleText` | Next=`..._CLEAR` | ✗ | 더러운 메쉬 |
| `..._CLEAR` | `Repeatable` | `SimpleText` | **PostCapture=`..._MEMORY`** | **✓** | 깨끗한 메쉬 |
| `..._MEMORY` | `Repeatable` | **`WorldStory`** | — | ✗ | 촬영 후 메쉬 |

**플레이 순서는 상호작용 → 사진 → 상호작용이다.**
`CLEAR` 는 일부러 `WorldStory` 가 아니다 — 가족 대화는 촬영을 거쳐 `MEMORY` 에 도달해야 나온다.
대신 `CLEAR` 에서도 F 가 반응하도록 `Repeatable` + `SimpleText` 로 두어 "사진을 찍어라" 안내를 띄운다.
(F 를 눌러도 아무 일도 없으면 고장난 오브젝트로 읽힌다.)

`MEMORY` 를 `Repeatable` 로 두면 대화를 여러 번 들을 수 있고,
완료 판정은 태그가 대신한다 — 아래 참고.

### 3) `DA_TutorialFlow_<레벨이름>` 만들기

우클릭 → Miscellaneous → Data Asset → **`BalhwajeomTutorialFlow`**

```
Locks On Start   = 잠글 Runtime.Lock.* (없으면 비움)
Stage Root Tag   = Tutorial.Stage
Release Locks On Finish = true
Steps            = 아래 표 참고
```

스텝 하나의 필드:

| 필드 | 뜻 |
|---|---|
| `Step ID` | 로그·디버그용 이름 |
| `Stage Tag` | 선택. 다른 시스템이 단계를 알아야 할 때만 |
| `Remove On Enter` | 스텝 진입 시 제거할 태그 → **여기에 `Runtime.Lock.*` 를 넣어 해금** |
| `Grant On Enter` | 스텝 진입 시 추가할 태그 |
| `Complete When All Tags` | 전부 있으면 다음 스텝 |
| `Complete When Any Tags` | 하나라도 있으면 다음 스텝 |
| `Complete When` (고급) | 위 둘로 표현 못 하는 조건(NOT 등)의 탈출구 |
| `Dim Mode` | `Off` / `Always`(아이콘 유도) / `FollowInteractPrompt`(바라볼 때만) |
| `Hint Target` | 깜빡일 대상. `None` / `InteractPrompt`(`[F]`) / `PhotoCameraIcon` / `TabletIcon` |
| `Dim Opacity` | 암전 세기 |

세 조건은 **AND** 로 묶이고, 비어 있는 조건은 건너뛴다.
셋 다 비우면 `AdvanceStep()` 을 부르기 전까지 그 스텝에 머문다(마지막 스텝의 표준 모양).

### 4) 레벨에 디렉터 배치

`ABalhwajeomTutorialDirector` 를 **1개만** 놓고 `Flow` 를 지정한다.
(`GetTutorialDirector` 는 월드에서 처음 찾은 하나를 쓴다.)

문을 잠그려면 `ABalhwajeomGateDoorActor` 를 놓고
`Door Interaction → Unlock Requires Tags` 와 `Locked Label` / `Unlocked Label` 을 채운다.

**건드릴 필요 없는 것**: 튜토리얼 레이어 위젯, `WBP_HUID`, `WBP_Interact`, 잠금 컴포넌트, 디렉터 C++.

---

## 4-1. 워크드 예제 — 새 맵에 오브젝트 3개 + 문

`Level_Chapter02` 에 오브젝트 세 개를 놓고, 셋을 다 처리해야 열리는 문을 두는 경우.
ID 는 `OBJ_02_001~003` 으로 잡는다.

### 먼저 알아야 할 것: 문은 오브젝트를 모른다

```
오브젝트 3개  ──(상태가 바뀔 때 태그를 뿌림)──▶  StoryStateSubsystem
                                                        │
문  ──(이 태그 3개가 다 있으면 열림)────────────────────┘
```

문에는 오브젝트 참조가 없고, 오브젝트에도 문 참조가 없다.
**둘 사이의 계약은 태그 이름뿐이다.** 그래서 오브젝트를 늘리거나 순서를 바꿔도
문의 `Unlock Requires Tags` 목록만 고치면 되고, 반대도 마찬가지다.

### 무엇을 "완료"로 볼지 먼저 정한다

문을 걸 태그는 목표에 따라 다르다. 셋 중 하나를 고른다.

| 문을 열 조건 | 쓸 태그 |
|---|---|
| 먼지만 털면 됨 | `Evidence.State.STATE_02_00X_CLEAR` |
| 사진까지 찍어야 함 | `Evidence.Photographed.OBJ_02_00X` |
| 대화까지 들어야 함 | **`Evidence.StoryHeard.OBJ_02_00X`** |
| 특정 사진의 빈칸 문장까지 풀어야 함 | **`Evidence.SentenceSolved.PHOTO_02_003`** |

room3 튜토리얼은 `Evidence.SentenceSolved.PHOTO_01_003`을 쓴다.

### 1) 태그 등록 — `Config/DefaultGameplayTags.ini`

```ini
+GameplayTagList=(Tag="Evidence.State.STATE_02_001_CLEAR",DevComment="")
+GameplayTagList=(Tag="Evidence.State.STATE_02_002_CLEAR",DevComment="")
+GameplayTagList=(Tag="Evidence.State.STATE_02_003_CLEAR",DevComment="")
+GameplayTagList=(Tag="Evidence.Photographed.OBJ_02_001",DevComment="")
+GameplayTagList=(Tag="Evidence.Photographed.OBJ_02_002",DevComment="")
+GameplayTagList=(Tag="Evidence.Photographed.OBJ_02_003",DevComment="")
+GameplayTagList=(Tag="Evidence.StoryHeard.OBJ_02_001",DevComment="")
+GameplayTagList=(Tag="Evidence.StoryHeard.OBJ_02_002",DevComment="")
+GameplayTagList=(Tag="Evidence.StoryHeard.OBJ_02_003",DevComment="")
+GameplayTagList=(Tag="Evidence.SentenceSolved.PHOTO_02_003",DevComment="")
```

`Tutorial.Stage.*` 는 **다시 만들 필요 없다.** 한 번에 한 디렉터만 돌기 때문에 맵끼리 공유해도 된다.

### 2) DataTable — 오브젝트당 3행

`DT_EvidenceDefinitions` (3행):

| ObjectID | InitialStateID |
|---|---|
| `OBJ_02_001` | `STATE_02_001_DUST` |
| `OBJ_02_002` | `STATE_02_002_DUST` |
| `OBJ_02_003` | `STATE_02_003_DUST` |

`DT_EvidenceStates` (오브젝트당 3행 = 9행). `OBJ_02_001` 기준:

| StateID | ObjectID | Behavior | Presentation | Next / PostCapture | bCanCapture | PhotoID |
|---|---|---|---|---|---|---|
| `STATE_02_001_DUST` | `OBJ_02_001` | `ChangeState` | `SimpleText` | Next=`STATE_02_001_CLEAR` | ✗ | — |
| `STATE_02_001_CLEAR` | `OBJ_02_001` | `None` | `SimpleText` | PostCapture=`STATE_02_001_MEMORY` | **✓** | `PHOTO_02_001` |
| `STATE_02_001_MEMORY` | `OBJ_02_001` | `Repeatable` | **`WorldStory`** | — | ✗ | `PHOTO_02_001` |

`DT_Photos` (3행): `PHOTO_02_001~003` 에 `WorldStoryCues` 를 채운다.
**첫 큐의 `StartTimeSeconds` 는 0** 이어야 한다.

> Row Name 은 `StateID` / `ObjectID` / `PhotoID` 와 **글자 그대로 같아야** 한다.
> 다르면 `Balhwajeom.Investigation.DataValidation` 이 잡아준다.

### 3) 플로우 에셋

`DA_TutorialFlow_Room2` 를 **복제**해서 `DA_TutorialFlow_Chapter02` 로 만들고
각 스텝의 태그 목록만 `_02_` 로 바꾼다. 스텝 구조(5개)는 그대로 쓸 수 있다.

| 스텝 | 바꿀 곳 |
|---|---|
| `DustTeach` | `Complete When Any Tags` → `..._02_00X_CLEAR` 3개 |
| `PhotoPrompt` | 그대로 (모드 태그라 맵 무관) |
| `Photograph` | `Complete When All Tags` → `Evidence.Photographed.OBJ_02_00X` 3개 |
| `CompleteFamilyPhoto` | `Remove On Enter` → `Runtime.Lock.Tablet`<br>`Complete When All Tags` → `Evidence.SentenceSolved.PHOTO_02_003` |
| `Done` | 그대로 |

두 번째 맵부터는 카메라·태블릿을 이미 배웠을 테니, `Locks On Start` 를 비우고
`PhotoPrompt` 스텝을 지우면 잠금 없이 진행 단계만 쓰는 플로우가 된다.

### 4) 레벨 배치

1. `ABalhwajeomEvidenceActor` 를 3개 놓고 인스턴스마다 **`Object ID` 만** 다르게
   (`OBJ_02_001` / `002` / `003`)
2. 각 액터의 `Evidence Mesh` 에 메쉬를 넣고, `Story Faces Player` 를 켜면
   3D 대화 텍스트가 플레이어 쪽을 본다
3. `ABalhwajeomTutorialDirector` **1개** — `Flow = DA_TutorialFlow_Chapter02`
4. `ABalhwajeomGateDoorActor` 를 문 자리에 놓는다

### 5) 문 연결 — 여기가 전부다

`ABalhwajeomGateDoorActor` 의 디테일 패널에서:

```
Door Interaction ▸ Unlock Requires Tags
    Evidence.SentenceSolved.PHOTO_02_003

Gate Door ▸ Locked Label     = [열 수 없는 문]
Gate Door ▸ Unlocked Label   = [F] 문 열기
Gate Door ▸ Opened Label     = (비움)

Door Mesh                    = 문 메쉬
Door ▸ Open Yaw Angle        = -90
```

room3 튜토리얼 출구(`GateDoor_Exit`)는 `Locked Feedback Stages`를 위에서부터 평가해
처음 완료되지 않은 단계의 문구를 `WBP_Check`에 표시한다.

| 순서 | 완료 조건 | 잠긴 문 상호작용 문구 |
|---|---|---|
| 1 | 세 액자의 `Evidence.State.STATE_01_00*_CLEAR` | `[F]를 눌러 아직 조사하지 않은 액자를 살펴보자.` |
| 2 | 세 액자의 `Evidence.Photographed.OBJ_01_00*` | `우클릭으로 카메라를 켜고, 아직 찍지 않은 액자를 촬영해 보자.` |
| 3 | `Evidence.SentenceSolved.PHOTO_01_003` | `[TAB]으로 태블릿을 열고, 여동생 폴더의 가족 사진 추리를 완성해 보자.` |

앞 단계가 남아 있으면 뒤 단계 태그가 일부 존재해도 앞 단계 문구를 우선한다.
단계 배열이 비어 있는 기존 문은 `WBP_Check`에 작성된 기본 문구를 그대로 사용한다.

**문의 피벗이 경첩 위치여야 한다.** 액터가 자기 피벗을 중심으로 돌기 때문에,
피벗이 문 한가운데면 문이 가운데서 회전한다. 메쉬 피벗이 가운데라면
액터를 경첩 위치에 놓고 `Door Mesh` 컴포넌트를 옆으로 오프셋한다.

잠긴 동안에는 `CanInteract()` 가 false 라 **`[F]` 프롬프트 자체가 안 뜬다.**
가까이 가면 `[열 수 없는 문]` 라벨만 보인다.

### 6) 확인

DataTable 연결이 깨졌으면 `Balhwajeom.Investigation.ConfiguredDataValidation` 에서 먼저 잡힌다.
`Room2TutorialFlowTest.cpp` 를 복사해 ID 만 바꾸면 레벨을 열지 않고도
새 맵의 흐름 전체(먼지털기 → 촬영 → 사진 문장 완성 → 문 개방)를 검증할 수 있다.

### 새로운 종류의 잠금이 필요할 때만 C++

1. `StoryStateTags.h/.cpp` 에 `Runtime.Lock.<X>` 네이티브 태그 추가
2. 막을 컴포넌트에 `FGameplayTagContainer BlockedByTags` + `IsLockedByStoryState()` 추가
   (`UBalhwajeomTabletComponent` 를 그대로 복사)
3. 생성자에서 `BlockedByTags.AddTag(...)`, 진입 함수 맨 앞에서 차단

---

## 5. 런타임 API

### 스태틱 (위젯용, 디렉터가 없어도 안전)

| 함수 | 반환 |
|---|---|
| `GetTutorialDirector(WorldContext)` | 없으면 null |
| `GetTutorialDimOpacity(WorldContext)` | 0~1. 모드·프롬프트 알파까지 반영된 최종값 |
| `GetTutorialHighlightPulse(WorldContext)` | 0~1 깜빡임. **디렉터가 없으면 1** (곱해도 무해) |
| `GetTutorialHintTarget(WorldContext)` | None / PhotoCameraIcon / TabletIcon |

### 인스턴스

| 함수 | 설명 |
|---|---|
| `SetFlow(Flow)` | 시작 전 플로우 교체 (챕터·난이도별) |
| `StartFlow()` / `AdvanceStep()` | 시작 / 조건 무시하고 다음 스텝 |
| `AbortFlow()` | **중단 + 걸린 잠금 전부 해제.** 스킵·복구 경로 |
| `SetLockActive(LockTag, bActive)` | 잠금을 임의로 걸거나 풀기 |
| `GetCurrentStepID()` / `IsFlowActive()` | 상태 조회 |
| `OnStepChanged` / `OnFlowFinished` | 연출·사운드 훅 |

**태블릿을 원하는 시점에 풀기**: `SetLockActive(Runtime.Lock.Tablet, false)` 한 줄.

---

## 6. 알아두면 좋은 동작

- **빨리감기**: 스텝 진입 직후 조건을 1회 평가한다. 이미 만족된 스텝은 자동 통과하므로,
  진행 중간에 플로우를 시작해도 올바른 스텝에서 시작한다.
- **잠금은 진입만 막는다**: 카메라 모드 안에서 잠금이 걸려도 나올 수 있다. 갇히지 않는다.
- **암전·깜빡임은 탐색 모드에서만**: `Runtime.Player.Mode` 가 Exploration 이 아니면 둘 다 꺼진다.
  스텝 전이 타이밍과 무관하게 보장되므로 디렉터가 멈춰도 카메라 모드 위에 암전이 남지 않는다.
  3D 아이템 조사 중에도 마찬가지다.
- **`StartFlow()` 는 BeginPlay 이후에**: 디렉터는 다이나믹 델리게이트로 태그 변화를 듣는데,
  `AActor::ProcessEvent` 는 월드의 액터 초기화 전에는 UFUNCTION 호출을 **조용히 버린다.**
  `bAutoStartOnBeginPlay` 를 쓰면 안전하다. GameMode 의 `InitGame` 같은 더 이른 시점에서 부르지 말 것.

---

## 7. 디버깅 순서

"아무 일도 안 일어난다" 증상일 때 위에서부터 확인한다.

1. **태그가 ini에 있는가** — 에디터 태그 피커에 안 보이면 그 태그는 존재하지 않는 것이다
2. **`Evidence.State.<StateID>` 의 StateID 가 DataTable Row Name 과 정확히 같은가**
3. **디렉터가 레벨에 1개 있고 `Flow` 가 지정됐는가** — 없으면 출력 로그에 경고가 남는다
4. **`GetCurrentStepID()` 가 기대한 스텝인가** — 아니면 앞 스텝의 완료 조건이 문제다
5. **암전이 안 보이면**: PC 의 `Tutorial Focus Widget Class` 가 비었는지, 플레이어 모드가 Exploration 인지
6. **아이콘 깜빡임이 안 보이면**: PC 의 `PlayerHUDWidgetClass` 가 비었거나,
   `Photo Camera Icon Names` / `Tablet Icon Names` 후보 중 HUD 에 실제로 있는 이름이 하나도 없다
   (room2 기준 `WBP_HUID` 의 `Image_Camera` / `Image_TAB`)
7. **`[F]` 깜빡임이 안 보이면**: PC 의 `Interaction Prompt Fade Target Name` 이
   프롬프트 위젯에 **실제로 있는 위젯 이름**인지 확인한다. 없으면 출력 로그에 경고가 남고
   아무것도 페이드하지 않는다. 스텝의 `Hint Target` 이 `InteractPrompt` 인지도 확인.
8. **위젯 에셋 이름을 바꿨다면 C++ 경로도 같이 고쳐야 한다.**
   `BalhwajeomCameraPlayerController` 와 `BalhwajeomEvidenceCameraHUD` 생성자가
   `ConstructorHelpers` 로 경로 문자열을 들고 있다. **에셋 리네임은 이 문자열을 따라가지 않고,
   조용히 null 이 되어 위젯이 아예 생성되지 않는다.**

---

## 8. 남은 이슈

1. **`Maps/Test` 폴더는 `.gitignore` 에 있다** (`.gitignore:78`).
   room2 의 튜토리얼 배치는 **버전 관리되지 않는다.** 팀에 공유하려면
   gitignore 를 풀거나 맵을 추적되는 경로로 옮겨야 한다.
2. **메쉬가 플레이스홀더다.** 사진은 `/Engine/BasicShapes/Cube`, 촬영 후 액자는 `Plane` 이다.
   흐름 확인에는 충분하지만 아트 교체가 필요하다 (`DT_EvidenceStates.StateMesh` 만 바꾸면 된다).
3. **room2 에 PlayerStart 가 2개** (`(0,-1440)`, `(-710,-2370)`) 다.
   GameMode 가 어느 쪽을 고를지 결정적이지 않다. 사진 세 장은 `(-570, -1820 ~ -2645)` 벽에 있으므로
   `PlayerStart2` 에서 시작하는 편이 가깝다.
4. **`DT_Photos` 는 JSON 내보내기가 크래시한다** (중첩 큐 구조체 때문).
   이 테이블은 스크립트로 수정할 수 없고 에디터에서 직접 편집해야 한다.
5. **`Balhwajeom.Investigation.DataValidation` 테스트가 실패 중** — 이 작업 이전부터
   (HEAD 기준선에서도) 실패한다. `CHAR_TEST` 에 `bIsFolderStatement` Statement 행이 없는
   테스트 픽스처 문제로, 이 시스템과 무관하다.
6. **세이브 로드 시 태그 복원 미확인** — `UStoryStateSubsystem::CurrentStateTags` 는 `Transient` 다.
   로드 후 `Evidence.State.*` 가 재발행되지 않으면 튜토리얼이 처음부터 다시 시작된다.
   디렉터의 빨리감기가 완화해 주지만 근본 해결은 아니다.
