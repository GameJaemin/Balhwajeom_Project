# 상단 알림 배너 + 긴급 탈출 확인창 설계

> 작성일: 2026-09-18
> 대상 레벨: `/Game/Levels/room3`
> 기반: `UBalhwajeomTutorialOverlayWidget` / `BalhwajeomTutorialOverlayLayout`
> **상태: 구현 완료 (2026-09-18). 에디터 빌드 성공, 자동화 회귀 0건. 수동 검증(5.2) 미실시.**
>
> **설계와 달라진 점**
> 1. 레이아웃을 별도 `…Layout` 네임스페이스로 분리하지 않고 각 위젯의 `EnsureFallbackLayout()`
>    안에 넣었다. 튜토리얼 쪽이 분리한 이유는 **에디터 스크립트가 그걸로 WBP를 생성**하기 때문인데,
>    여기서는 WBP를 만들지 않으므로 나눌 이유가 없다.
> 2. 확인창에 `CanAcceptInput()` 가드를 넣었다. 페이드 인 도중의 클릭이 통과하면 플레이어가
>    무엇에 동의했는지 보기도 전에 순간이동한다.
> 3. 암전 전후에 `BlackHoldSeconds`(0.15초) 정지 구간을 넣었다. 텔레포트와 같은 프레임에
>    페이드백을 시작하면 카메라 붐이 자리잡기 전 첫 프레임이 보인다.
> 4. 테스트를 3개로 나눴다(`NotificationBanner.Schedule`, `EmergencyEscape.DoorChoice`,
>    `EmergencyEscape.Transform`).

---

## 0. 결론 먼저

**결정 1 — `.uasset`(WBP)을 새로 만들지 않는다. 튜토리얼 오버레이와 똑같이 C++로 짓는다.**

"튜토리얼 레이아웃 WBP를 복사해서" 라고 하셨는데, **실제 레이아웃은 WBP가 아니라 C++에 있다.**
[`BalhwajeomTutorialOverlayLayout::Build()`](../../../Source/Balhwajeom/Private/Tutorial/BalhwajeomTutorialOverlayLayout.cpp)가 위젯 트리 전체를 짓고,
`WBP_TutorialOverlay`는 거기서 **생성된 결과물**이다. 런타임 클래스도 WBP 없이 뜨면
[`EnsureFallbackLayout()`](../../../Source/Balhwajeom/Private/Tutorial/BalhwajeomTutorialOverlayWidget.cpp)으로 같은 트리를 직접 만든다.

그래서 새 화면 두 개도 같은 방식으로 만든다. 얻는 것:

| | 이유 |
|---|---|
| **병합 충돌 0** | `.uasset`은 병합이 안 된다([README 규칙 6](../../../README.md)). 지금 4명이 각자 브랜치에서 작업 중이고 오늘이 마감이다 |
| **바로 동작** | 에디터에서 에셋을 만들고 부모 클래스를 지정하는 수작업이 필요 없다 |
| **나중에 WBP 가능** | 두 클래스 모두 `Blueprintable`. 디자이너가 나중에 WBP를 만들면 `BindWidgetOptional` 이름만 맞추면 그쪽이 이긴다 |

**결정 2 — 두 화면의 성격이 정반대이므로 입력 처리도 정반대로 만든다.**

| | 알림 배너 | 긴급 탈출 확인창 |
|---|---|---|
| 입력 | **절대 안 받음** (`HitTestInvisible`) | 마우스만 받음 (`FInputModeUIOnly`) |
| 화면 | 상단 일부 | 전체 (딤 + 블러) |
| 종료 | 시간이 지나면 자동 | 버튼을 눌러야 |
| 게임 | 계속 진행됨 | 이동/시점 잠김 |

**결정 3 — 탈출 목적지는 레벨에서 가장 가까운 `ABalhwajeomGateDoorActor` 앞이다.**
`.umap`을 건드리지 않고, 문이 여러 개인 맵에서도 알아서 동작한다. 실제 이동은
`AActor::TeleportTo()`에 맡긴다 — 도착 지점이 막혀 있으면 엔진이 빈 자리를 찾아준다.
**끼임을 푸는 기능이 도착지에서 다시 끼면 안 되기 때문에** 직접 `SetActorLocation` 하지 않는다.

---

## 1. 알림 배너

### 1.1 레이아웃

```text
CanvasPanel (root)
 └ SB_Banner        SizeBox, 상단 중앙 앵커, TopMargin 만큼 내림
    └ VB_Banner     VerticalBox
       ├ BRD_TopLine     Border  1px  흰색
       ├ BRD_Body        Border  검정 30% 알파
       │   └ TXT_Message TextBlock  한 줄, 가운데 정렬
       └ BRD_BottomLine  Border  1px  흰색
```

위/아래 선을 `Border`의 테두리 브러시가 아니라 **별도 Border 두 개**로 만든다. 좌우로 선이 안 나가고,
두께를 픽셀 단위로 정확히 통제할 수 있다.

배너 전체는 `ESlateVisibility::HitTestInvisible`. 게임 중에 뜨는 물건이라 클릭을 절대 가로채면 안 된다.

### 1.2 수명

```text
Show(Text)
  └ FadeIn(0.25s) → Hold(2.5s) → FadeOut(0.6s) → 뷰포트에서 제거
```

진행 중에 `Show()`가 다시 불리면 **문구를 갈아끼우고 처음부터 다시 시작**한다. 배너를 쌓지 않는다 —
화면 맨 위 한 줄이라는 게 이 UI의 정체성이다.

곡선은 순수 함수로 뺀다(테스트 가능):

```cpp
namespace BalhwajeomNotificationBanner
{
    /** 0 → 1 → 1 → 0. 스케줄을 다 쓰면 0을 돌려준다. */
    float ResolveOpacity(float Elapsed, float FadeIn, float Hold, float FadeOut);
    bool  IsFinished(float Elapsed, float FadeIn, float Hold, float FadeOut);
}
```

### 1.3 호출 경로

```text
어디서든
  BalhwajeomNotification::Show(WorldContext, FText)      ← static 헬퍼
       ↓ 로컬 플레이어 컨트롤러를 찾아서
  UBalhwajeomNotificationPresenter::ShowNotification()   ← 컨트롤러의 컴포넌트
       ↓
  UBalhwajeomNotificationBannerWidget::Show()
```

프레젠터는 `EnsureTutorialOverlayPresenter()`와 같은 방식으로 `BeginPlay`에서 붙인다.
`ShowNotification`은 `BlueprintCallable`이라 블루프린트에서도 바로 쓸 수 있다.

### 1.4 문 해금 자동 연동

`ABalhwajeomGateDoorActor`는 이미 매 틱 `RefreshLabelForLockState()`에서 잠금 상태 변화를
에지 검출하고 있다(`bLastKnownUnlocked`). 거기에 한 줄만 붙인다.

```cpp
UPROPERTY(EditAnywhere, Category = "Gate Door|Text")
FText UnlockedNotificationText;   // 예: "문이 열리는 소리가 난 것 같다.."
```

**`bHasLabelState`가 이미 true일 때의 false→true 전이에서만** 띄운다. 그래야 처음부터 열린
문이 `BeginPlay` 첫 틱에 알림을 쏘지 않는다. 한 번 띄우면 다시 안 띄운다.

비워두면 아무 일도 안 일어난다 — 기존 배치 문들은 설정 0개로 그대로 동작한다.

---

## 2. 긴급 탈출 확인창

### 2.1 레이아웃

튜토리얼 오버레이와 같은 딤 + 블러 위에:

```text
CanvasPanel (root)
 └ OVL_Root        Overlay, 화면 전체
    ├ BG_Blur      BackgroundBlur  6.0
    ├ BRD_Dim      Border  검정 65%
    └ SB_Content   SizeBox  폭 800
       └ VB_Content VerticalBox  가운데
          ├ TXT_Title      TextBlock  "긴급 탈출"
          ├ TXT_Message    TextBlock  "지형에 끼어 움직일 수 없나요?\n방 문 앞으로 이동합니다."
          └ HB_Buttons     HorizontalBox
             ├ BTN_Confirm  Button  "네"
             └ BTN_Cancel   Button  "아니오"
```

### 2.2 흐름

```text
Ctrl + Alt + Backspace
  └ 확인창 페이드 인 (0.35s) · 마우스 커서 ON · FInputModeUIOnly · 이동/시점 잠금

  [아니오]
    └ 페이드 아웃 (0.25s) → 입력 복구 → 끝

  [네]
    └ 확인창 페이드 아웃
       → WBP_ScreenFade 로 암전 (0.4s)
          → 카메라 모드 강제 종료 · 속도 0 · TeleportTo(문 앞)
          → 암전 해제 (0.5s) → 입력 복구
```

암전은 `UBalhwajeomScreenFadeWidget`(`/Game/Balhwajeom/UI/Title/WBP_ScreenFade`)을 그대로 쓴다.
인트로가 이미 쓰고 있는 위젯이라 새로 만들 게 없다.

**순간이동은 반드시 완전 암전 상태에서 한다.** 텔레포트가 보이면 그 자체가 버그처럼 읽힌다.

### 2.3 목적지 계산

순수 함수로 뺀다(테스트 가능):

```cpp
namespace BalhwajeomEmergencyEscape
{
    /** 후보 중 PlayerLocation에 가장 가까운 것의 인덱스. 비면 INDEX_NONE. */
    int32 ResolveNearestDoorIndex(const FVector& PlayerLocation, const TArray<FVector>& DoorLocations);

    /** 문 트랜스폼 + 로컬 오프셋 → 월드 위치, 그리고 문을 바라보는 야우. */
    FTransform ResolveEscapeTransform(const FTransform& DoorTransform, const FVector& LocalOffset);
}
```

- 오프셋은 `EscapeLocalOffset`(기본 `(150, 0, 0)`)으로 에디터에서 조절한다. 문 액터는 힌지에 놓이므로
  실제 "앞"이 어느 쪽인지는 배치마다 다르다 — 코드로 추측하지 않고 값으로 맞춘다.
- 도착 야우는 **문을 바라보도록** 잡는다. 탈출 직후 플레이어가 문을 보고 있는 게 방향 감각에 좋다.
- 문이 하나도 없으면 `APlayerStart`로 폴백한다.

### 2.4 입력

```ini
+ActionMappings=(ActionName="EmergencyEscape",bShift=False,bCtrl=True,bAlt=True,bCmd=False,Key=BackSpace)
```

`Config/DefaultInput.ini` 한 줄. Escape는 이미 `ExitCameraMode`가 쓰고 있어 피했고,
3키 조합이라 오입력이 사실상 불가능하다.

> **한계**: `FInputModeUIOnly`가 켜진 동안(태블릿 열림, 상호작용 모달)에는 액션이 오지 않는다.
> 끼임은 게임플레이 중에 생기는 문제라 실사용에는 문제없다고 판단했다.

---

## 3. 기각한 대안

| 대안 | 기각 이유 |
|---|---|
| 에디터 Python으로 WBP 2개 생성 | `.uasset` 2개가 늘고 병합 충돌 위험이 생긴다. C++ 폴백이 이미 있어 얻는 게 없다 |
| 알림 배너를 DataTable로 | 새 `DT_.uasset`이 필요하고, 지금 용도는 문 해금 한 건이다 |
| 탈출 지점 전용 마커 액터 배치 | `room3.umap` 수정 → 충돌 위험. 문 액터에서 유도하면 레벨을 안 건드린다 |
| 탈출을 `SetActorLocation`으로 | 도착 지점이 막혀 있으면 또 낀다. `TeleportTo`는 빈 자리를 찾아준다 |
| 확인창을 기존 `InteractionModal`로 | 그 모달은 조사 텍스트 전용이고 Yes/No 개념이 없다 |

---

## 4. 파일 목록

| # | 파일 | 내용 |
|---|---|---|
| 1 | `Public/UI/BalhwajeomNotificationBanner.h` + `Private/UI/…cpp` | **신규.** 페이드 스케줄 순수 함수 2개 |
| 2 | `Public/UI/BalhwajeomNotificationBannerWidget.h` + cpp | **신규.** 배너 위젯 + 자체 레이아웃 |
| 3 | `Public/UI/BalhwajeomNotificationPresenter.h` + cpp | **신규.** 컨트롤러 컴포넌트 + `BalhwajeomNotification::Show` |
| 4 | `Public/UI/BalhwajeomConfirmPromptWidget.h` + cpp | **신규.** 예/아니오 확인창 + 자체 레이아웃 |
| 5 | `Public/UI/BalhwajeomEmergencyEscape.h` + cpp | **신규.** 목적지 계산 순수 함수 2개 |
| 6 | `Public/UI/BalhwajeomEmergencyEscapePresenter.h` + cpp | **신규.** 입력 → 확인창 → 암전 → 텔레포트 |
| 7 | `CameraSystem/BalhwajeomCameraPlayerController.*` | 컴포넌트 2개 생성, `EmergencyEscape` 액션 바인딩 |
| 8 | `Interaction/BalhwajeomGateDoorActor.*` | `UnlockedNotificationText` + 해금 전이에서 알림 |
| 9 | `Config/DefaultInput.ini` | ActionMapping 한 줄 |
| 10 | `Private/UI/Test/NotificationBannerTest.cpp` | **신규.** |
| 11 | `Private/UI/Test/EmergencyEscapeTest.cpp` | **신규.** |

`.uasset` · `.umap` · DataTable **변경 없음.**

---

## 5. 검증

### 5.1 자동화

`Balhwajeom.UI.NotificationBanner`
1. 스케줄 각 구간의 불투명도 (0 → 1 → 1 → 0)
2. 페이드 구간이 0이어도 0으로 나누지 않음
3. 스케줄을 다 쓰면 `IsFinished` true, 그 전엔 false
4. Hold 구간 내내 정확히 1.0

`Balhwajeom.UI.EmergencyEscape`
5. 후보가 비면 `INDEX_NONE`
6. 가장 가까운 문을 고름 (순서 무관)
7. 동점이면 낮은 인덱스
8. 로컬 오프셋이 문의 회전을 따라 회전함
9. 도착 야우가 문을 향함

### 5.2 수동 (room3)

| # | 절차 | 기대 |
|---|---|---|
| N1 | 튜토리얼 사진 3장을 찍어 문을 해금 | 상단에 "문이 열리는 소리가 난 것 같다.." 가 떴다가 사라진다 |
| N2 | N1 직후 계속 플레이 | 배너가 클릭/입력을 전혀 가로채지 않는다 |
| N3 | 배너가 떠 있는 동안 다시 해금 이벤트 | 배너가 쌓이지 않고 하나만 갱신된다 |
| E1 | Ctrl+Alt+Backspace | 화면이 막히고 커서가 나오며 네/아니오가 보인다 |
| E2 | 아니오 | 부드럽게 사라지고 이동이 바로 된다 |
| E3 | 네 | 암전 → 문 앞 → 암전 해제. 텔레포트 순간이 보이지 않는다 |
| E4 | 카메라 모드에서 E3 | 카메라가 내려간 상태로 문 앞에 선다 |
| E5 | 실제로 낀 상태에서 E3 | 빠져나온다. 도착 지점에서 다시 끼지 않는다 |
| E6 | 문이 없는 맵(Intro 등)에서 E1 | 크래시 없이 PlayerStart로 가거나 아무 일도 없다 |

---

## 6. 엣지 케이스

| 상황 | 처리 |
|---|---|
| 확인창이 떠 있는데 또 단축키 | 무시 (이미 열려 있음) |
| 탈출 중(암전 중)에 단축키 | 무시 |
| 문이 하나도 없음 | `APlayerStart` 폴백, 그것도 없으면 아무 일 없음 |
| 배너 표시 중 레벨 종료 | `EndPlay`에서 뷰포트에서 제거 |
| 탈출 후 태블릿/카메라 상태 | 카메라 모드는 강제 종료, 태블릿은 UI 모드라 애초에 단축키가 안 온다 |
| 문구가 비어 있음 | 배너를 띄우지 않는다 |
