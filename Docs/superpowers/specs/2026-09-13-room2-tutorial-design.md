# 튜토리얼 / 단계 진행 구조 설계 (첫 적용: Room2)

> 첫 적용 레벨: `/Game/Balhwajeom/Maps/Test/room2`
> 작성일: 2026-09-13
> **상태: 구현 완료. Room2 플레이 가능 (2026-09-13).**
> 사용법·다른 레벨 적용법은 [Docs/Systems/Tutorial-Flow.md](../../Systems/Tutorial-Flow.md)를 볼 것.
> 이 문서는 설계 기록이며, 아래 내용 중 일부는 구현 중 바뀌었다.
>
> **설계와 달라진 점**
> 1. 잠금 태그를 **네이티브 태그**로 만들어 ini 등록이 필요 없어졌다.
> 2. 암전을 `ETutorialDimMode`(Off / Always / FollowInteractPrompt) 열거형으로 데이터화했다.
> 3. 튜토리얼 레이어를 **C++ `UBalhwajeomTutorialFocusWidget`** 으로 만들어 위젯 BP가 필요 없어졌고,
>    하이라이트가 HUD 아이콘의 브러시·사각형을 자동으로 복사하므로 수동 정렬도 사라졌다.
> 4. 스텝 조건을 `FGameplayTagContainer` 두 개(All/Any)로 받도록 넓혔다. 쿼리는 고급 옵션으로 남겼다.
> 5. **새 증거 `OBJ_TUT_01~03`을 만들지 않았다.** 삼남매 사진 `OBJ_01_001~003`과 가족대화 자막이
>    이미 프로젝트에 있었으므로 그대로 쓰고, 002/003에 빠져 있던 촬영 후 상태만 채웠다.
> 6. 가족대화 완료 판정은 `Evidence.State.*`가 아니라 **`Evidence.StoryPlayed.<StateID>`** 태그를 쓴다.
>    `Repeatable` 월드 스토리는 상태를 바꾸지 않아 다시 들을 수 있어야 하기 때문이다.
> 7. 문은 `ABalhwajeomGateDoorActor`(신규 C++ 액터)가 담당한다. `ConditionalData` 라벨 방식 대신
>    액터가 잠금 상태에 따라 라벨을 직접 고른다.
> 8. **`DustRest` 스텝을 뺐다.** 아래 7.1 표는 사진 세 장을 모두 턴 뒤에야 카메라를 해금하지만,
>    실제로는 **한 장만 털면 바로 카메라 아이콘이 강조된다.** 나머지 두 장은 `Photograph` 단계에서
>    턴다 — 먼지 상태는 촬영이 막혀 있어 어차피 F를 먼저 써야 하므로 흐름이 끊기지 않는다.
> 목표: room2 튜토리얼을 **한 번만 만들고, 다른 맵에서는 데이터 에셋만 새로 만들어 재사용**한다.
> 기존 조사/촬영 시스템은 그대로 두고 그 위에 얹는다. 새 서브시스템은 만들지 않는다.

---

## 0. 결론 먼저

재사용 가능한 구조를 만드는 데 핵심 결정은 두 가지다.

**결정 1 — 잠금은 "해금 태그가 있으면 열림"이 아니라 "잠금 태그가 있으면 막힘"으로 만든다.**

| 모델 | 기본 상태 | 기존 맵 영향 |
|---|---|---|
| ❌ 해금 태그 모델 (`Unlocked.X` 있어야 열림) | 잠김 | **모든 맵이 해금 태그를 부여해야 함** |
| ✅ **잠금 태그 모델 (`Runtime.Lock.X` 있으면 막힘)** | 열림 | **없음. 설정 0개로 전부 정상 동작** |

Level_Main·프로토타입 맵 등 기존 레벨을 **한 곳도 건드리지 않는다.**

**결정 2 — 암전은 기존 위젯에 넣지 말고, 뷰포트 ZOrder 5에 별도 레이어로 띄운다.**

말씀하신 대로 "검은 Image + alpha + ZOrder"가 맞다. 다만 **어느 레이어에 놓느냐**가
"[F]는 바라볼 때만 강조" 문제를 **코드 없이 공짜로 풀어준다.** (→ 6장)

구조는 3층이다.

```
[3] 프레젠테이션  WBP_TutorialFocus (ZOrder 5)  ← 스텝이 "무엇을 강조할지"만 받음. 맵을 모름
        ▲ OnStepChanged
[2] 진행         ABalhwajeomTutorialDirector + DA_TutorialFlow_*  ← 맵마다 데이터만 교체
        ▲ Runtime.Lock.* / Evidence.* 태그
[1] 잠금         PhotoCameraComponent / TabletComponent / DoorInteractionComponent
                 ← 태그만 보고 막는다. 튜토리얼을 전혀 모름
```

1층은 튜토리얼 전용이 아니다. **스토리 진행 잠금 전반에 그대로 쓰인다.**

---

## 1. 확인된 기존 기반 (재사용 대상)

코드를 확인한 결과 아래는 **이미 동작한다. 새로 만들 필요 없다.**

| 필요한 것 | 이미 있는 것 | 위치 |
|---|---|---|
| "F로 상태 바꾸기" (먼지털기) | `EEvidenceInteractionBehavior::ChangeState` + `NextStateID` | InvestigationEnums.h |
| 촬영 후 메쉬 교체 (하얀 액자) | `FEvidenceStateDefinition::PostCaptureStateID` + `StateMesh` | EvidenceDefinitions.h |
| 가족대화 (3D 월드 텍스트 + 음성) | `EEvidenceInteractionPresentation::WorldStory` + `FPhotoDefinition::WorldStoryCues` / `StoryVoice` | PhotoDefinitions.h |
| 상태별 라벨 (`[F] 먼지 털기`) | 상태의 `FarLabel/MidLabel/NearLabel` → `ApplyInvestigationState`가 `InspectionComponent`에 주입 | BalhwajeomEvidenceActor.cpp |
| 촬영 완료 태그 | `Evidence.Photographed.<ObjectID>` 자동 발행 | StoryStateSubsystem.cpp |
| 조건부 라벨 (`[열 수 없는 문]` ↔ `[문 열기]`) | `UInspectionComponent::ConditionalData` | InspectionComponent.h |
| **`[F]` 프롬프트를 "바라볼 때만" 표시** | `ShouldShowInteractionPrompt()` — 포커스 + Close 거리 + `CanInteract()` 전부 확인 후 페이드 | BalhwajeomCameraPlayerController.cpp |
| 카메라/태블릿 모드 배타 태그 | `Runtime.Player.Mode.*` | StoryStateTags.cpp |
| **레벨 진행 디렉터 액터 선례** | `ABalhwajeomIntroFlowActor` | BalhwajeomIntroFlowActor.h |
| 태블릿 일시 차단 | `SetTabletInteractionEnabled` (IntroFlow가 이미 사용) | BalhwajeomTabletComponent.cpp |

room2 실행 경로 확인:
`room2.umap` → `BP_OrbitViewGameMode` → `BP_OrbitViewPlayerController`(부모 `ABalhwajeomCameraPlayerController`) → 폰 `BP_OrbitViewCharacter_Legacy`(부모 `ABalhwajeomCameraCharacter`, `PhotoCameraComponent`/`TabletComponent` 보유).

---

## 2. 뷰포트 레이어 지도 ⭐ (암전 설계의 전제)

소스 전체의 `AddToViewport` / `AddToPlayerScreen`을 전수 확인한 결과다.

| ZOrder | 위젯 | 내용 | 생성 위치 |
|---|---|---|---|
| 0 | **`WB_HUID`** (PlayerHUD) | `Image_0`=카메라 아이콘(`HUD_CAM`), `Image_1`=태블릿 아이콘 | `EnsurePlayerHUD()` |
| 1 | `WB_HUD2` | 베드 메모리 HUD | `EnsureBedMemoryHUD()` |
| **5** | **`WBP_TutorialFocus`** | **신규 — 암전 + 하이라이트** | (추가 예정) |
| 10 | **`WB_Interact`** | 중앙 점 + `[F]` 프롬프트 텍스트(`TextBlock_50`) | `EnsureInteractionPrompt()` |
| 100 | `WBP_EvidenceFocusGuide` | 카메라 모드 포커스 가이드 | `EvidenceCameraHUD` |
| 100 | 태블릿 위젯 | | `TabletComponent` |
| 250 | `WBP_CapturePhoto` | 촬영 카드 연출 | `EvidenceCameraHUD` |
| 1000 / 2000 / 9999 | 메인메뉴 / 시네마틱 / 스크린 페이드 | | `IntroFlowActor` |

**여기서 나오는 결론:** ZOrder 5에 전체 화면 검은 Image를 두면

- **아래로 어두워지는 것**: 3D 월드 전체 + `WB_HUID`의 카메라/태블릿 아이콘
- **위에 밝게 남는 것**: `WB_Interact`의 **중앙 점과 `[F]` 프롬프트**, 태블릿, 촬영 카드, 페이드

즉 **"화면이 어두워지고 `[F]`만 하얗게 뜬다"가 ZOrder 하나로 끝난다.**
그리고 그 프롬프트는 `ShouldShowInteractionPrompt()`가 **이미 "바라보고 있고, 가깝고, 상호작용 가능할 때만"** 띄운다.

> 처음 고민하셨던 "플레이어가 오브젝트를 바라볼 때만 표시되는 건데 어떻게 처리하지"는
> **레이어 순서만으로 해결된다. 추가 코드가 필요 없다.**
> (이전 초안에서 제안했던 포커스 변경 델리게이트 `C6`은 **필요 없어져서 삭제했다.**)

### 2.1 왜 `WB_Interact`나 `WB_HUID` 안에 넣지 않는가

말씀하신 두 후보 모두 **기술적으로는 된다.** 다만 이런 대가가 붙는다.

| 위치 | 되는 것 | 걸리는 것 |
|---|---|---|
| `WB_Interact` (ZO 10) | 형제 순서로 `[F]`를 암전 위에 올릴 수 있음 | 카메라 아이콘(ZO 0)이 암전 아래라 **하이라이트 복제본을 `WB_Interact` 안에 또 넣어야 함**. PC가 이 위젯의 `RenderOpacity`/`Visibility`를 매 틱 직접 조작하므로(`InteractionPromptFadeTarget` 로직) 암전이 그 페이드에 얽힐 위험 |
| `WB_HUID` (ZO 0) | 카메라 아이콘 강조는 형제 순서로 해결 | `SetGameplayPresentationEnabled(false)`나 베드 메모리 크로스페이드로 **`WB_HUID` 전체가 `Collapsed`되면 암전도 같이 사라짐**. 베드 HUD(ZO 1)가 암전 위에 뜸 |
| **별도 ZO 5 (권장)** | 위 둘 다 없음 + `[F]` 자동 밝음 | 위젯 클래스 1개와 PC에 생성 코드 ~10줄 추가 |

**구현 난이도는 사실상 같다** (어디에 놓든 검은 Image + alpha + 순서). 차이는 **결합도**뿐이고,
재사용 관점에서 별도 레이어가 명확히 낫다 — 다른 맵에서 `WB_HUID`/`WB_Interact`를 손대지 않아도 되기 때문이다.

---

## 3. 태그 네임스페이스

### 3.1 잠금 태그 — 맵 무관, 영구 (`Config/DefaultGameplayTags.ini`)

```ini
; --- 런타임 능력 잠금. 태그가 "있으면" 막힌다. 없으면 평소대로 동작. ---
+GameplayTagList=(Tag="Runtime.Lock",DevComment="런타임 능력 잠금 루트")
+GameplayTagList=(Tag="Runtime.Lock.PhotoCamera",DevComment="우클릭 카메라 모드 진입 차단")
+GameplayTagList=(Tag="Runtime.Lock.Tablet",DevComment="TAB 태블릿 열기 차단")
```

### 3.2 증거 상태 진입 태그 — 자동 발행 (C2)

`Evidence.State.<StateID>` 형태. 기존 `Evidence.Photographed.<ObjectID>`와 같은 방식.
**튜토리얼 전용이 아니다** — 본편 증거 상태 전이도 전부 조건 쿼리에서 쓸 수 있게 된다.

### 3.3 room2 전용 태그

```ini
; 튜토리얼 스테이지
+GameplayTagList=(Tag="Tutorial.Stage",DevComment="튜토리얼 스테이지 루트(배타)")
+GameplayTagList=(Tag="Tutorial.Stage.DustTeach",DevComment="F 상호작용 학습")
+GameplayTagList=(Tag="Tutorial.Stage.DustRest",DevComment="나머지 액자 먼지털기")
+GameplayTagList=(Tag="Tutorial.Stage.PhotoPrompt",DevComment="카메라 모드 유도")
+GameplayTagList=(Tag="Tutorial.Stage.Photograph",DevComment="사진 3장 촬영")
+GameplayTagList=(Tag="Tutorial.Stage.Talk",DevComment="가족대화 3개")
+GameplayTagList=(Tag="Tutorial.Stage.Done",DevComment="튜토리얼 종료")

; 튜토리얼 증거 3종 (자동 발행되므로 등록만 필요)
+GameplayTagList=(Tag="Evidence.Photographed.OBJ_TUT_01",DevComment="삼남매 사진 촬영 완료")
+GameplayTagList=(Tag="Evidence.Photographed.OBJ_TUT_02",DevComment="가족사진 2 촬영 완료")
+GameplayTagList=(Tag="Evidence.Photographed.OBJ_TUT_03",DevComment="가족사진 3 촬영 완료")

+GameplayTagList=(Tag="Evidence.State.TUT_01_Clean",DevComment="삼남매 사진 먼지털기 완료")
+GameplayTagList=(Tag="Evidence.State.TUT_02_Clean",DevComment="가족사진 2 먼지털기 완료")
+GameplayTagList=(Tag="Evidence.State.TUT_03_Clean",DevComment="가족사진 3 먼지털기 완료")
+GameplayTagList=(Tag="Evidence.State.TUT_01_Talked",DevComment="삼남매 사진 가족대화 완료")
+GameplayTagList=(Tag="Evidence.State.TUT_02_Talked",DevComment="가족사진 2 가족대화 완료")
+GameplayTagList=(Tag="Evidence.State.TUT_03_Talked",DevComment="가족사진 3 가족대화 완료")
```

> `RequestGameplayTag(..., ErrorIfNotFound=false)`를 쓰므로 **ini에 없는 태그는 조용히 무시된다.**
> 등록 누락이 가장 흔한 버그 원인이다. 다른 맵에 이 구조를 쓸 때도 **태그 등록이 첫 단계**다.

---

## 4. C++ 변경 (총 5곳, 전부 작음)

| # | 변경 지점 | 성격 | 재사용성 |
|---|---|---|---|
| C1 | `UBalhwajeomPhotoCameraComponent` 잠금 체크 | 필수 | 전 맵 공용 |
| C2 | `UStoryStateSubsystem` 상태 진입 태그 발행 | 필수 | 전 프로젝트 공용 |
| C3 | `UBalhwajeomTabletComponent` 잠금 체크 | 필수 | 전 맵 공용 |
| C4 | `UDoorInteractionComponent` 태그 쿼리 잠금 | 필수 | 문 인스턴스별 |
| C5 | `ABalhwajeomTutorialDirector` + `UBalhwajeomTutorialFlow` | 필수 | **맵마다 데이터만 교체** |
| C7 | `ABalhwajeomCameraPlayerController` — 튜토리얼 레이어 생성 + 프롬프트 알파 게터 | 필수 | 전 맵 공용 |

> 이전 초안의 `C6`(포커스 변경 델리게이트)은 ZOrder 레이어링으로 불필요해져 **삭제**했다.

### C1 / C3. 잠금 체크 — 공통 패턴

두 컴포넌트에 **똑같은 3줄**을 넣는다.

```cpp
/** Any of these tags present in the story state blocks this ability. Empty = never blocked. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock")
FGameplayTagContainer BlockedByTags;   // 디폴트: { Runtime.Lock.PhotoCamera } / { Runtime.Lock.Tablet }

UFUNCTION(BlueprintPure, Category = "Lock")
bool IsLockedByStoryState() const;     // StoryState->HasAnyStateTags(BlockedByTags)
```

**`ToggleCameraMode()` 최상단:**

```cpp
// 잠금은 "진입"만 막는다. 이미 모드 안이면 나가는 건 항상 허용해야 플레이어가 갇히지 않는다.
if (!bIsInCameraMode && IsLockedByStoryState())
{
    OnCameraModeBlocked.Broadcast();   // HUD 힌트 연출 훅 (BlueprintAssignable)
    return;
}
```

**`ToggleTablet()`** — 기존 `bTabletInteractionEnabled` 체크 바로 옆:

```cpp
if (!bTabletInteractionEnabled || (!bTabletOpen && IsLockedByStoryState()))
{
    return;
}
```

> 기존 `bTabletInteractionEnabled`(IntroFlow 사용 중)는 **건드리지 않는다.**
> 그건 "컷신 동안 잠깐 끔", 태그 잠금은 "진행 상태에 따른 잠금"이다. 목적이 다르므로 공존시킨다.

**왜 컴포넌트 디폴트에 태그를 박아도 안전한가**: 태그가 스토리 상태에 *없으면* 항상 열려 있다.
잠금을 거는 주체(디렉터)가 없는 맵은 자동으로 전부 해제 상태다. 설정 0개.

### C2. 증거 상태 진입 태그 — `UStoryStateSubsystem`

기존 `HandlePhotoCaptured` / `AddPhotographedEvidenceTag`와 **완전히 같은 패턴**:

```cpp
UFUNCTION()
void HandleEvidenceStateChanged(FGuid ChangedInstanceID, FName PreviousStateID, FName NewStateID);

void AddEvidenceStateTag(FName StateID);   // "Evidence.State.<StateID>"
```

`Initialize()`에서 `InvestigationSubsystem->OnEvidenceStateChanged.AddUniqueDynamic(...)`,
`Deinitialize()`에서 `RemoveDynamic(...)`. (기존 두 델리게이트 바로 옆)

이거 하나로 **먼지털기 완료 / 가족대화 완료 / 앞으로 나올 모든 상태 전이**가 조건 쿼리에서 쓸 수 있는 태그가 된다.

### C4. 문 잠금 — `UDoorInteractionComponent`

```cpp
/** Empty query = always unlocked. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Gate")
FGameplayTagQuery UnlockQuery;

UFUNCTION(BlueprintPure, Category = "Door")
bool IsUnlocked() const;
```

`CanInteract()`에 `&& IsUnlocked()` 한 줄.
`RequestInspect`가 문 컴포넌트를 `IWorldInteractable`보다 **먼저** 검사하므로 BP로는 우회 불가 — 여기서 막아야 한다.

**부수 효과(좋은 쪽)**: `ShouldShowInteractionPrompt()`가 문에 대해 `DoorInteraction->CanInteract()`를 그대로 쓰므로,
**잠긴 문 앞에서는 `[F]` 프롬프트가 아예 안 뜬다.** 별도 처리 불필요.

room2 쿼리: `MatchAllTags{ Evidence.State.TUT_01_Talked, TUT_02_Talked, TUT_03_Talked }`

**라벨은 코드 불필요** — 문의 `InspectionComponent`에 `ConditionalData` 1행 + 기본 필드:

| # | Condition Query | NearLabel |
|---|---|---|
| 0 | 위 3개 MatchAll | `[F] 문 열기` |
| — | (기본 `NearLabel` 필드) | `[열 수 없는 문]` |

`ConditionalData`는 **첫 매칭 행이 이긴다.** 해제 조건을 0번에 둬야 한다.

### C7. `ABalhwajeomCameraPlayerController` 추가분

```cpp
/** Tutorial dim/highlight layer. Created once, ZOrder 5 — above the HUD, below WB_Interact. */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Tutorial")
TSubclassOf<UUserWidget> TutorialFocusWidgetClass;

UFUNCTION(BlueprintCallable, Category = "UI|Tutorial") void EnsureTutorialFocusLayer();
UFUNCTION(BlueprintPure,     Category = "UI|Tutorial") UUserWidget* GetTutorialFocusLayer() const;

/** 0..1 fade alpha of the [F] prompt text. Lets the dim fade in perfect sync with the prompt. */
UFUNCTION(BlueprintPure, Category = "UI|Interaction")
float GetInteractionPromptAlpha() const;   // FadeTarget ? GetRenderOpacity() : 0.f
```

`EnsurePlayerHUD()` 옆에 `AddToViewport(5)` 한 줄. `SetGameplayPresentationEnabled`에도
다른 HUD 레이어들과 같이 포함시켜 컷신 중 같이 숨긴다.

`GetInteractionPromptAlpha()`가 중요한 이유는 6.3에 있다.

---

## 5. C5. 재사용의 핵심 — 플로우 데이터 에셋 + 디렉터

### 5.1 `UBalhwajeomTutorialFlow : UPrimaryDataAsset`

**맵마다 이 에셋만 새로 만들면 된다. 코드는 건드리지 않는다.**

```cpp
UENUM(BlueprintType)
enum class ETutorialHintTarget : uint8
{
    None,              // 아이콘 강조 없음 ([F] 프롬프트는 레이어 순서로 항상 밝음)
    PhotoCameraIcon,   // WB_HUID의 카메라 아이콘
    TabletIcon,        // WB_HUID의 태블릿 아이콘
};

UENUM(BlueprintType)
enum class ETutorialDimMode : uint8
{
    Off,                    // 암전 없음
    Always,                 // 스텝 내내 암전 (아이콘 유도용)
    FollowInteractPrompt,   // [F] 프롬프트가 뜰 때만 암전 — 바라볼 때만 강조
};

USTRUCT(BlueprintType)
struct FBalhwajeomTutorialStep
{
    UPROPERTY(EditAnywhere) FName StepID;                        // 디버그/로그용

    /** 선택. 배타 그룹(StageRootTag)에 설정할 태그. 비우면 태그를 쓰지 않는다. */
    UPROPERTY(EditAnywhere) FGameplayTag StageTag;

    UPROPERTY(EditAnywhere) FGameplayTagContainer GrantOnEnter;
    /** 여기에 Runtime.Lock.* 를 넣어 해금한다. */
    UPROPERTY(EditAnywhere) FGameplayTagContainer RemoveOnEnter;

    /** 만족되면 다음 스텝으로. 비우면 수동 Advance 전까지 유지. */
    UPROPERTY(EditAnywhere) FGameplayTagQuery CompleteWhen;

    // --- 연출 지시. 맵 이름도 태그 이름도 모른다. ---
    UPROPERTY(EditAnywhere) ETutorialDimMode DimMode = ETutorialDimMode::Off;
    UPROPERTY(EditAnywhere) ETutorialHintTarget HintTarget = ETutorialHintTarget::None;
    UPROPERTY(EditAnywhere, meta = (ClampMin="0.0", ClampMax="1.0")) float DimOpacity = 0.65f;
    UPROPERTY(EditAnywhere, meta = (MultiLine = "true")) FText HintText;
};

UCLASS(BlueprintType)
class BALHWAJEOM_API UBalhwajeomTutorialFlow : public UPrimaryDataAsset
{
    /** 플로우 시작 시 걸 잠금. 예: Runtime.Lock.PhotoCamera, Runtime.Lock.Tablet */
    UPROPERTY(EditAnywhere) FGameplayTagContainer LocksOnStart;

    /** 배타 스테이지 그룹 루트. StageTag를 쓸 때만 필요. 예: Tutorial.Stage */
    UPROPERTY(EditAnywhere) FGameplayTag StageRootTag;

    UPROPERTY(EditAnywhere) TArray<FBalhwajeomTutorialStep> Steps;
};
```

### 5.2 `ABalhwajeomTutorialDirector : AActor`

`ABalhwajeomIntroFlowActor`와 같은 성격의 레벨 배치 액터. 맵당 1개.

```cpp
UPROPERTY(EditAnywhere, Category = "Tutorial") TObjectPtr<UBalhwajeomTutorialFlow> Flow;
UPROPERTY(EditAnywhere, Category = "Tutorial") bool bAutoStartOnBeginPlay = true;

UFUNCTION(BlueprintCallable, Category = "Tutorial") void StartFlow();
UFUNCTION(BlueprintCallable, Category = "Tutorial") void AdvanceStep();   // 디버그 스킵
UFUNCTION(BlueprintCallable, Category = "Tutorial") void AbortFlow();     // 남은 잠금 전부 해제
UFUNCTION(BlueprintPure,     Category = "Tutorial") FName GetCurrentStepID() const;

/** 언제든 임의의 잠금을 걸거나 푼다. 태블릿을 미리 풀고 싶을 때 이걸 쓴다. */
UFUNCTION(BlueprintCallable, Category = "Tutorial")
void SetLockActive(FGameplayTag LockTag, bool bActive);

UPROPERTY(BlueprintAssignable, Category = "Tutorial")
FOnTutorialStepChanged OnStepChanged;   // (FBalhwajeomTutorialStep NewStep) — 튜토리얼 레이어가 바인드
```

동작:

```
StartFlow()
  ├─ LocksOnStart 를 StoryState 에 추가
  ├─ StoryState.OnStateTagAdded / OnStateTagRemoved 구독
  └─ EnterStep(0)

EnterStep(i)
  ├─ StageTag 유효 → SetExclusiveStateTag(StageRootTag, StageTag)
  ├─ RemoveOnEnter 제거  (← 여기서 Runtime.Lock.* 가 풀린다)
  ├─ GrantOnEnter 추가
  ├─ OnStepChanged.Broadcast(Step)
  └─ EvaluateCurrentStep()        ← 진입 즉시 1회 평가 (빨리감기)

태그 변경 이벤트 → EvaluateCurrentStep()
  └─ CompleteWhen 유효 && MatchesStateQuery 통과 → EnterStep(i+1)
```

**설계 포인트 3가지**

1. **빨리감기(idempotent)** — `EnterStep`이 진입 직후 `CompleteWhen`을 1회 평가하므로
   이미 만족된 스텝은 자동 통과한다. 세이브 로드·디버그 재시작·중간 진입에서 흐름이 깨지지 않는다.
2. **`AbortFlow()`가 남은 잠금을 전부 해제** — 스킵하거나 버그로 멈춰도 플레이어가 갇히지 않는다.
3. **`SetLockActive`가 잠금/해제의 유일한 공개 창구** — "태블릿 미리 풀기"는
   `SetLockActive(Runtime.Lock.Tablet, false)` 한 줄이다. BP·치트·디버그 위젯 어디서든.

### 5.3 왜 디렉터를 C++로 두는가

쿼리 평가·스텝 순회·빨리감기·구독 해제를 BP로 짜면 맵마다 복붙된다.
C++에 한 번 두면 **다른 맵은 `DA_TutorialFlow_*` 에셋만 만들면 끝**이다. 이게 재사용성의 본체다.
연출(애니메이션·사운드)은 `OnStepChanged`를 받아 BP에서 자유롭게 한다.

---

## 6. UI — 튜토리얼 포커스 레이어

### 6.1 `WBP_TutorialFocus` 위젯 트리 (CanvasPanel, 아래 형제가 위에 그려짐)

```
CanvasPanel_Root                      (Visibility = HitTestInvisible)
├─ Img_Dim            검정 단색, 전체 화면(Anchor 0,0 ~ 1,1, Offset 0)   ← Collapsed
├─ Img_CamHighlight   HUD_CAM,     WB_HUID의 Image_0과 동일 Slot 값       ← Collapsed
└─ Img_TabHighlight   HUD_TAB__1_, WB_HUID의 Image_1과 동일 Slot 값       ← Collapsed
```

- `Img_Dim`은 **`RenderOpacity`로 밝기를 조절**한다 (Brush 색의 A가 아니라). 페이드가 쉬워진다.
- 하이라이트는 `WB_HUID`의 아이콘과 **픽셀 단위로 겹쳐야 한다.** Anchors / Offsets / Alignment / Size를
  그대로 복사할 것. 어긋나면 암전 위아래로 아이콘이 두 개로 보인다.
- `Img_TabHighlight`를 지금 같이 만들어 두는 이유: 다음 맵에서 태블릿 튜토리얼을 넣을 때
  **위젯을 다시 안 건드리기 위해서**다. `ETutorialHintTarget` 항목에 대응하는 슬롯을 미리 채워둔다.
- 루트를 `HitTestInvisible`로 둘 것. `Visible`이면 암전이 마우스 입력을 먹는다.

### 6.2 표시 규칙 — 맵을 모른다

이 위젯은 **태그 이름도 맵 이름도 모른다.** 디렉터의 `OnStepChanged`만 받는다.

```
OnStepChanged(Step)  →  CurrentStep 저장
  ├─ Img_CamHighlight : Step.HintTarget == PhotoCameraIcon
  └─ Img_TabHighlight : Step.HintTarget == TabletIcon

매 Tick (또는 0.1s 타이머):
  TargetAlpha =
      ( PlayerMode != Exploration )                    ? 0
    : ( Step.DimMode == Off )                          ? 0
    : ( Step.DimMode == Always )                       ? Step.DimOpacity
    : ( Step.DimMode == FollowInteractPrompt )         ? Step.DimOpacity * PC.GetInteractionPromptAlpha()

  Img_Dim.RenderOpacity = FInterpTo(현재, TargetAlpha, DeltaTime, 8.0)
```

**`PlayerMode != Exploration`이면 무조건 0인 규칙이 중요하다.**
카메라 모드·태블릿·아이템 조사 중에는 암전이 절대 남지 않는다.
"우클릭해서 1인칭 모드로 바뀌면 화면 어두워지는 거 사라지게" 요구사항이 이 한 줄로 보장된다 —
스텝 전이 타이밍에 의존하지 않으므로 **디렉터가 멈춰도 안전하다.**

### 6.3 `FollowInteractPrompt` — `[F]` 강조의 정답

`DimMode = FollowInteractPrompt`면 암전 알파가 **`[F]` 프롬프트의 페이드 알파에 그대로 비례**한다.

- 대상을 **바라보면**: 프롬프트가 0→1로 페이드 인 → 암전도 같이 0→0.65로 들어옴
- **시선을 돌리면**: 프롬프트가 1→0 → 암전도 같이 빠짐
- **완벽히 동기화된다.** 별도 페이드 로직도, 깜빡임 보정도 필요 없다.
- 프롬프트 조건(`ShouldShowInteractionPrompt`)에 이미 포커스·Close 거리·`CanInteract()`가 들어 있으므로,
  **잠긴 문이나 상호작용 불가 상태에서는 암전도 안 생긴다.**

이것이 `GetInteractionPromptAlpha()` 게터 하나(2줄)를 추가하는 이유다.

### 6.4 선택 폴리시

- 암전 중 `HintText`를 화면 하단에 띄우고 싶으면 `WBP_TutorialFocus`에 `TextBlock`을 추가한다.
- 대상 오브젝트 자체를 밝히고 싶으면 `EvidenceMesh`의 `Render CustomDepth` + 스텐실로
  포스트프로세스 아웃라인을 추가한다. **필수는 아니다** — 6.3으로 이미 읽힌다.
- `OnCameraModeBlocked`(C1)에 바인드해 "잠긴 동안 우클릭" 시 카메라 아이콘 흔들기 같은 피드백을 붙일 수 있다.

---

## 7. Room2 튜토리얼 흐름 (최종)

### 7.1 `DA_TutorialFlow_Room2`

```
LocksOnStart = { Runtime.Lock.PhotoCamera, Runtime.Lock.Tablet }
StageRootTag = Tutorial.Stage
```

| # | StepID | StageTag | DimMode | Hint | RemoveOnEnter | CompleteWhen |
|---|---|---|---|---|---|---|
| 0 | `DustTeach` | `…DustTeach` | **FollowInteractPrompt** | None | — | MatchAny{ `Evidence.State.TUT_01_Clean`, `TUT_02_Clean`, `TUT_03_Clean` } |
| 1 | `DustRest` | `…DustRest` | Off | None | — | MatchAll{ 위 3개 전부 } |
| 2 | `PhotoPrompt` | `…PhotoPrompt` | **Always** | **PhotoCameraIcon** | **`Runtime.Lock.PhotoCamera`** | MatchAny{ `Runtime.Player.Mode.PhotoCamera` } |
| 3 | `Photograph` | `…Photograph` | Off | None | — | MatchAll{ `Evidence.Photographed.OBJ_TUT_01/02/03` } |
| 4 | `Talk` | `…Talk` | **FollowInteractPrompt** | None | — | MatchAll{ `Evidence.State.TUT_01_Talked`, `TUT_02_Talked`, `TUT_03_Talked` } |
| 5 | `Done` | `…Done` | Off | None | **`Runtime.Lock.Tablet`** | (비움 — 종료 스텝) |

### 7.2 플레이어가 보는 흐름

| 단계 | 화면 | 가능한 입력 |
|---|---|---|
| **0. DustTeach** | 평소엔 평범. 액자를 **바라보면** 화면이 어두워지며 `[F] 먼지 털기`만 밝게 떠오름 | **F만** (우클릭·TAB 잠김) |
| | 한 개라도 털면 → 스텝 1 | |
| **1. DustRest** | 암전 없음. 자유롭게 나머지 2개 탐색 | F만 |
| | 3개 전부 털면 → 스텝 2 | |
| **2. PhotoPrompt** | **화면 전체 암전 + 카메라 아이콘만 하얗게** | **우클릭 해금** |
| | 우클릭으로 1인칭 진입 → **암전 즉시 사라짐**, 스텝 3 | |
| **3. Photograph** | 평범한 카메라 모드 | 촬영 |
| | 찍을 때마다 액자가 **하얀 액자 메쉬로 교체**. 3장 다 찍으면 → 스텝 4 | |
| **4. Talk** | 액자를 바라보면 암전 + `[F] 가족의 대화 듣기` | F |
| | 누르면 3D 월드 텍스트로 가족대화 재생. 3개 다 들으면 → 스텝 5 | |
| **5. Done** | 평범 | **태블릿 해금**, 문이 `[F] 문 열기`로 바뀜 |

**`DustTeach` / `DustRest`로 쪼갠 이유**: 처음 F를 가르칠 때는 암전 강조가 효과적이지만,
나머지 2개를 *찾아다니는* 동안 화면이 계속 어두우면 탐색이 답답해진다.
첫 1회만 강조하고 나머지는 평범하게 둔다. **마음에 안 들면 스텝 0의 `CompleteWhen`을
MatchAll로 바꾸고 스텝 1을 지우면 3개 내내 강조된다 — 데이터만 수정.**

**문 상태**: 스텝 5 이전에는 `UnlockQuery` 불만족이므로 `[열 수 없는 문]`이 뜨고
**`[F]` 프롬프트 자체가 안 뜬다**(C4의 부수 효과). 스텝 5 이후 `[F] 문 열기`.

**태블릿을 더 일찍 풀려면**: `SetLockActive(Runtime.Lock.Tablet, false)` 호출,
또는 `LocksOnStart`에서 `Runtime.Lock.Tablet`을 빼면 끝.

### 7.3 `DT_EvidenceDefinitions` — 3행

| ObjectID | ObjectName | InitialStateID |
|---|---|---|
| `OBJ_TUT_01` | 삼남매 사진 | `TUT_01_Dusty` |
| `OBJ_TUT_02` | 가족사진 2 | `TUT_02_Dusty` |
| `OBJ_TUT_03` | 가족사진 3 | `TUT_03_Dusty` |

### 7.4 `DT_EvidenceStates` — 오브젝트당 4행 (총 12행)

`OBJ_TUT_01` 기준. 02/03 동일 패턴.

| StateID | Behavior | Presentation | Next / PostCapture | bCanCapture | PhotoID | StateMesh | NearLabel |
|---|---|---|---|---|---|---|---|
| `TUT_01_Dusty` | `ChangeState` | `SimpleText` | Next=`TUT_01_Clean` | ✗ | — | (배치 메쉬 유지) | `[F] 먼지 털기` |
| `TUT_01_Clean` | `None` | `None` | PostCapture=`TUT_01_Framed` | **✓** | `PHOTO_TUT_01` | — | `카메라로 촬영하세요` |
| `TUT_01_Framed` | `ChangeState` | **`WorldStory`** | Next=`TUT_01_Talked` | ✗ | — | **`SM_Frame_White_01`** | `[F] 가족의 대화 듣기` |
| `TUT_01_Talked` | `None` | `None` | — | ✗ | — | (흰 액자 유지) | (빈칸) |

**코드에서 확인한 동작 근거:**

- `TUT_01_Clean.Behavior = None` → `BeginEvidenceInteraction`이 **false 반환**해 F가 먹지 않는다.
  "촬영만 가능한 상태"를 만드는 정석 방법이다. `ShouldShowInteractionPrompt()`도 false가 되어
  **`[F]` 프롬프트와 암전이 같이 사라진다** — 스텝 3에서 암전이 없는 것과 일관된다.
- `PostCaptureStateID`가 채워져 있으면 촬영 시 `AdvanceEvidenceStateAfterCapture`가 호출되고,
  **촬영 연출은 자체 WorldStory를 건너뛴다.** 사진 찍자마자 대화가 터지지 않고 다음 상태에서 F로 재생된다.
- `StateMesh`는 `ApplyStateVisuals`에서 교체 → **하얀 액자 전환은 데이터만으로** 된다.
- `WorldStory`는 `RequestInvestigationInteraction` 안에서만 재생된다(상태 진입만으로는 재생 안 됨).
- `Framed`를 `Once`가 아니라 **`ChangeState`로 두는 이유**: `Once`는 아무 이벤트도 쏘지 않아
  "대화 완료"를 감지할 수 없다. `ChangeState`는 `OnEvidenceStateChanged`를 브로드캐스트하므로 C2가 태그를 발행한다.

### 7.5 `DT_Photos` — 3행

| PhotoID | CharacterID | WorldStoryCues | StoryVoice |
|---|---|---|---|
| `PHOTO_TUT_01` | (지정 필요) | 가족대화 자막 큐 배열 | 대화 음성 |
| `PHOTO_TUT_02` | 〃 | 〃 | 〃 |
| `PHOTO_TUT_03` | 〃 | 〃 | 〃 |

> `WorldStoryCues[0].StartTimeSeconds`는 반드시 0. 음성 없이 자막만 쓰려면 `StoryVoice`를 비우면 타임드 텍스트로 재생된다.

### 7.6 레벨 배치 (`Maps/Test/room2`)

- 액자 3개 = `ABalhwajeomEvidenceActor` 파생 BP (`BP_Evidence_TutPhoto`), 인스턴스별 `ObjectID`만 다르게.
- 각 액터의 `StoryAnchor`를 플레이어가 설 위치 쪽으로 회전(+X가 읽는 방향).
- `CameraFocusPoint`는 액자 정면 중앙.
- `BP_TutorialDirector` 1개, `Flow = DA_TutorialFlow_Room2`.
- 문: `UDoorInteractionComponent` + `UInspectionComponent` 보유 BP, `UnlockQuery` + `ConditionalData` 설정.

---

## 8. 다른 맵에서 재사용하는 절차

구조가 완성되면 새 맵에 튜토리얼/단계 진행을 넣는 작업은 **코드 0줄**이다.

1. 새 증거의 `Evidence.State.*` / `Evidence.Photographed.*` / 스테이지 태그를 ini에 등록
2. `DT_EvidenceDefinitions` / `DT_EvidenceStates` / `DT_Photos`에 행 추가
3. `DA_TutorialFlow_<맵이름>` 새로 만들고 스텝 작성
4. 맵에 `BP_TutorialDirector` 1개 배치 + `Flow` 지정
5. 잠글 문에 `UnlockQuery` + `ConditionalData` 라벨 설정

`WBP_TutorialFocus` · `WB_HUID` · `WB_Interact` · 잠금 컴포넌트 · 디렉터 코드는 **전혀 손대지 않는다.**
새로운 종류의 잠금이 필요해지면 그때만 `Runtime.Lock.<X>` 태그 + 해당 컴포넌트에 C1과 같은 3줄을 추가한다.

---

## 9. 작업 순서

| 순서 | 작업 | 산출물 | 검증 |
|---|---|---|---|
| 1 | 잠금/스테이지/증거 태그 등록 | `DefaultGameplayTags.ini` | 에디터 태그 피커에 전부 보임 |
| 2 | **C2 상태 진입 태그 발행** | `StoryStateSubsystem` | 먼지털기 후 태그 발생 확인 |
| 3 | C1 + C3 잠금 체크 | Photo/Tablet 컴포넌트 | **Level_Main이 기존과 동일 동작** (회귀 확인) |
| 4 | C7 프롬프트 알파 게터 + 레이어 생성 | `CameraPlayerController` | 빈 위젯이 ZOrder 5에 뜸 |
| 5 | `WBP_TutorialFocus` 제작 | UI | 하이라이트가 `WB_HUID` 아이콘과 정확히 겹침 |
| 6 | C5 디렉터 + 플로우 에셋 클래스 | 신규 C++ 2개 | 빈 플로우로 BeginPlay 크래시 없음 |
| 7 | DT 3종 입력 | Evidence/States/Photos | — |
| 8 | `BP_Evidence_TutPhoto` + 액자 3개 배치 | room2 | F 먼지털기 → 라벨 전환 |
| 9 | `DA_TutorialFlow_Room2` + 디렉터 배치 | room2 | 시작 시 우클릭·TAB 무반응, 3개 털면 우클릭 동작 |
| 10 | 암전 튜닝 | 알파·페이드 속도 | 바라볼 때 암전, 1인칭 진입 시 즉시 해제 |
| 11 | 촬영 → 하얀 액자 | — | `PostCaptureStateID` 동작 |
| 12 | 가족대화 3D 텍스트 | `DT_Photos` 큐 | F로 재생, 완료 시 태그 |
| 13 | C4 문 잠금 + `ConditionalData` | Door | 대화 3개 전 `[열 수 없는 문]`, 후 `[문 열기]` |

**2번이 먼저인 이유**: 이후 모든 게이팅이 이 태그에 의존한다. 여기가 안 되면 전부 안 된다.
**3번의 회귀 확인이 중요한 이유**: 잠금 태그 모델의 전제가 "기존 맵 무영향"이므로 반드시 검증한다.
**4~5번이 6번보다 앞인 이유**: 레이어 순서가 의도대로 나오는지는 디렉터 없이도 확인 가능하다.
암전이 `[F]`를 덮어버리는 게 이 설계의 유일한 시각적 리스크이므로 **먼저 눈으로 확인하고 넘어간다.**

---

## 10. 남은 확인 / 리스크

1. **`Content/Levels/room2.umap`은 어떻게 할 것인가.** 정본은 `Maps/Test/room2.umap`로 확정됐지만,
   현재 작업 트리에서 수정된 쪽은 `Content/Levels/room2.umap`이다. 중복 맵을 정리하지 않으면
   다음 사람이 다시 헷갈린다 — 삭제 또는 정리 필요.
2. **세이브 로드 시 스토리 태그 복원.** `UStoryStateSubsystem::CurrentStateTags`는 `Transient`다.
   로드 직후 `Evidence.State.*` / `Evidence.Photographed.*`가 재발행되는지 미확인.
   디렉터의 빨리감기가 완화해 주지만, **태그가 복원되지 않으면 로드 후 튜토리얼이 처음부터 다시 시작된다.**
   이번 일정 밖이면 최소한 이슈로 남길 것.
3. **하얀 액자 메쉬(`SM_Frame_White_01`) 부재.** 아트 요청 필요.
4. **`DT_Photos`의 `CharacterID`** — 튜토리얼 사진 3장을 어느 캐릭터 폴더에 넣을지 미정.
   태블릿이 잠긴 동안은 안 보이지만 해금 후 폴더에 나타난다.
5. **`ETutorialHintTarget` 확장 시 위젯 수정 필요.** enum에 항목을 추가하면 `WBP_TutorialFocus`에
   대응 하이라이트를 추가해야 한다. 구조상 불가피하며, 6.1에서 태블릿 슬롯을 미리 만들어 빈도를 줄였다.
6. **`BP_OrbitViewPlayerController`가 `WB_HUID`를 BP 그래프에서 직접 생성하는지 확인 필요.**
   네이티브 `EnsurePlayerHUD()`가 `PlayerHUDWidgetClass`로 만드는 경로와 BP가 따로 만드는 경로가
   겹쳐 있으면 HUD가 2개 생겨 하이라이트 정렬이 틀어진다. 5번 작업 전에 한 번 열어볼 것.
