# 키워드 클릭 시 빈칸으로 날아가는 연출 설계

> 작성일: 2026-09-18
> 대상 코드: `UBalhwajeomTabletWidget`, `UBalhwajeomTabletWordChip`
> 적용 화면: 태블릿 사진 분석 퍼즐 / 진술서 반증 퍼즐
> **상태: 구현 완료 (2026-09-18). 에디터 빌드 성공, 자동화 회귀 0건. 수동 검증(6.2) 미실시.**
>
> **설계와 달라진 점**
> 1. 비행 레이어를 `RebuildWidget()` 오버라이드 대신 **`BuildLayer()`를 화면에 붙이기 전에 호출**하는 방식으로 만들었다.
>    `UBalhwajeomTabletWordChip`이 `Configure()`에서 자기 `WidgetTree`를 세우는 것과 같은 방식이라,
>    Blueprint 없는 네이티브 `UUserWidget`의 트리 생성 시점을 엔진 내부 동작에 맡기지 않는다.
> 2. 테스트를 `…KeywordFlight.Curve` / `…KeywordFlight.SlotChoice` 두 개로 나눴다.
> 3. **비행 칩에 `UBalhwajeomTabletWordChip::ApplyFlightStyle()`을 적용한다.** 퍼즐 후보 칩은
>    `bStatementStyle = true`로 만들어지는데 이 스타일은 호버 전까지 배경이 투명이라, 그냥 복제하면
>    **글자만** 날아간다. 드래그의 `DefaultDragVisual`이 이미 쓰고 있는 모습(`keyword_hover` 브러시 +
>    `FColor(255,237,217)` 틴트 + 검은 글씨)을 그대로 적용해, 클릭과 드래그가 같은 모양으로 보이게 했다.
> 4. 첫 플레이 확인 후 기본값 조정: `KeywordFlightDuration` `0.25 → 0.45`,
>    `KeywordFlightEaseExponent` `3.0 → 5.0`.

---

## 0. 결론 먼저

지금 클릭 경로는 [`HandleWordChipClicked()`](../../../Source/Balhwajeom/Private/Tablet/BalhwajeomTabletWidget.cpp)가 첫 빈칸을 찾아 곧장 `HandleSentenceBlankDropped()`를 부른다. 그 안에서 **제출 기록·빈칸 채우기·완성 판정이 한 프레임에 전부** 일어나므로 "띡띡" 채워진다.

**결정 1 — 날아가는 것은 클릭한 칩의 *복제본*이고, 화면 최상위 레이어에 그린다.**
원본 칩을 렌더 트랜스폼으로 옮기지 않는다. 칩은 `WB_PuzzleWords`(WrapBox) 안에 있어 조상 패널의 클리핑에 잘릴 수 있고, 키워드는 사용 후에도 목록에 남아야 하기 때문이다.

**결정 2 — 도착 지점이 정해지는 것과 실제로 채워지는 것을 분리한다.**
클릭 순간에는 **비행 목록에만 예약**하고 `ActiveSubmission`에는 쓰지 않는다. 착지할 때 비로소 기존 `HandleSentenceBlankDropped()`를 호출한다.
→ 완성 판정(`EvaluatePuzzleIfComplete`)은 `ActiveSubmission`이 다 찼을 때만 도는데, 그 기록 자체가 착지 시점에 생기므로 **"빈칸에 들어간 뒤 판정"이 구조적으로 보장**된다. 순서에 기대는 게 아니다.

**결정 3 — 위치는 ease-out(감속), 크기는 살짝 컸다가 제자리로.**
`Alpha = 1 − (1−t)³`. 초반에 확 튀어나가고 빈칸 앞에서 감속한다. 크기는 `1.15 → 1.0`으로 같은 곡선을 탄다.

드래그 경로는 **그대로 즉시 채운다.** 이미 커서로 칩을 빈칸까지 끌고 간 뒤라, 목록 위치에서 다시 날아가면 오히려 어색하다.

---

## 1. 현재 동작

```text
칩 클릭
  └ HandleWordChipClicked(WordID)
       ├ ActiveBlanksBySlot 키를 정렬
       ├ ActiveSubmission에 없는 첫 SlotIndex를 찾음
       └ HandleSentenceBlankDropped(SlotIndex, WordID, INDEX_NONE)
            ├ KeywordDropSound 재생
            ├ ActiveSubmission.SubmittedWords 갱신
            ├ Blank->SetFilled(...)          ← 즉시
            └ EvaluatePuzzleIfComplete()     ← 즉시
```

`EvaluatePuzzleIfComplete()`는 `SubmittedWords.Num() >= WordSlots.Num()`일 때만 `ValidateActivePuzzle()`로 넘어간다. 즉 **판정 기준은 이미 "제출 기록이 다 찼는가"**다. 그러니 제출 기록을 쓰는 시점만 착지로 미루면 요구사항 두 번째는 자동으로 만족된다.

칩은 사용해도 `WB_PuzzleWords`에서 사라지지 않는다([`RefreshPuzzleControls()`](../../../Source/Balhwajeom/Private/Tablet/BalhwajeomTabletWidget.cpp)가 획득한 모든 단어를 항상 그린다). 원본을 옮기면 안 되는 이유가 하나 더 있는 셈이다.

태블릿 위젯은 `AddToPlayerScreen(100)`으로 붙는다([BalhwajeomTabletComponent.cpp:414](../../../Source/Balhwajeom/Private/Tablet/BalhwajeomTabletComponent.cpp#L414)). 화면 공간 위젯이므로 뷰포트 좌표로 계산하면 정확히 겹친다.

---

## 2. 기각한 대안

| 대안 | 기각 이유 |
|---|---|
| 원본 칩에 렌더 트랜스폼을 걸어 이동 | WrapBox/ScrollBox 조상의 클리핑에 잘린다. 또 칩은 목록에 그대로 남아 있어야 한다. |
| 클릭 즉시 `ActiveSubmission`에 쓰고 판정만 미루기 | 요구사항 2를 "호출 순서"로만 보장하게 된다. 드래그·오답 리셋이 끼어들면 깨진다. 예약을 비행 목록이 들고 있으면 그 틈이 없다. |
| UMG 애니메이션(Widget Animation) 사용 | 시작·도착 좌표가 런타임에 정해지므로 트랙으로 표현할 수 없다. `.uasset` 편집도 필요해진다. |
| `WB_SentenceBuilder`에 임시 칩을 넣고 이동 | WrapBox는 자식을 자동 배치한다. 절대 위치를 줄 수 없다. |

---

## 3. 설계

### 3.1 비행 레이어

C++ 전용 `UBalhwajeomTabletKeywordFlightLayer : UUserWidget`. `RebuildWidget()`에서 `UCanvasPanel` 하나를 루트로 만든다. `.uasset`은 만들지 않는다.

```text
AddToPlayerScreen(KeywordFlightLayerZOrder)   // 기본 110
   └ CanvasPanel                              // HitTestInvisible
        └ 날아가는 칩 복제본 (CanvasPanelSlot, AutoSize, Alignment 0.5/0.5)
```

- **ZOrder 110**: 태블릿(100) 위, 촬영 카드(250) 아래.
- **HitTestInvisible**: 입력을 절대 먹지 않는다.
- 비행이 하나도 없으면 레이어를 화면에서 내리고 버린다. 유휴 상태에서 전체화면 위젯이 남지 않게.

### 3.2 좌표

`Alignment(0.5, 0.5)`로 **중심점만** 다루면 DPI 스케일 변환이 필요 없다.

```text
시작(절대) = ChipGeometry.LocalToAbsolute(LocalSize * 0.5)
도착(절대) = BlankGeometry.LocalToAbsolute(LocalSize * 0.5)
          ↓ USlateBlueprintLibrary::AbsoluteToViewport
CanvasPanelSlot.Position = Lerp(시작뷰포트, 도착뷰포트, Eased)
```

도착점은 **비어 있는 상태의 빈칸 중심**이다. 착지 후 빈칸이 단어 길이만큼 늘어나는데, WrapBox가 내용을 가운데 정렬하므로 글자는 칩이 내려앉은 그 자리에 나타난다.

두 위젯 중 하나라도 캐시된 지오메트리가 없으면(아직 한 번도 그려지지 않았으면) **비행 없이 즉시 채운다.** 연출 때문에 기능이 막히는 일은 없어야 한다.

### 3.3 곡선

```text
t     = clamp(Elapsed / Duration, 0, 1)
Eased = 1 − (1 − t)^Exponent           // Exponent 3.0 = cubic ease-out
크기  = Lerp(StartScale, 1.0, Eased)   // StartScale 1.15
```

`Exponent`가 클수록 초반이 더 급하다. `1.0`이면 등속. `StartScale = 1.0`이면 크기 연출이 꺼진다.

### 3.4 슬롯 예약

```text
HandleWordChipClicked(WordID)
  └ 목적지 = 정렬된 SlotIndex 중
       ActiveSubmission에도 없고, 비행 목록에도 없는 첫 번째
```

- 빠르게 두 번 클릭해도 두 번째 칩은 **다음** 빈칸으로 간다.
- 마지막 빈칸으로 향하던 칩이 착지해야 `ActiveSubmission`이 가득 차고, 그때 판정이 돈다.

### 3.5 착지

```text
TickKeywordFlights(DeltaTime)
  └ Elapsed >= Duration 인 비행:
       1. 비행 목록에서 제거      ← 예약 해제가 먼저
       2. 복제 칩 파괴
       3. HandleSentenceBlankDropped(TargetSlot, WordID, INDEX_NONE)
            → 소리 · 제출 기록 · SetFilled · EvaluatePuzzleIfComplete
```

드래그와 클릭이 **같은 착지 함수 하나로 수렴**한다. 스왑 규칙·오답 스타일·판정이 두 벌로 갈라지지 않는다.

`KeywordDropSound`는 클릭이 아니라 **착지**에 울린다. "떨어뜨리는" 소리이므로 키워드가 빈칸에 닿는 순간이 맞다.

### 3.6 취소

빈칸 위젯이 다시 만들어지면 비행 중인 칩의 목적지 포인터가 무효가 된다. 아래에서 전부 취소한다.

| 지점 | 이유 |
|---|---|
| `BuildSentenceBuilder()` | 오답 시 퍼즐을 통째로 다시 만든다 |
| `ClearSentenceBuilder()` | 퍼즐을 닫거나 다른 사진으로 넘어간다 |
| `NativeDestruct()` | 태블릿이 사라질 때 레이어도 같이 |
| `HandleSentenceBlankDropped()` 진입부 | 드래그가 비행 목적지 칸을 먼저 채운 경우 |

`ActiveBlanksBySlot`이 초기화되는 곳은 위 두 함수뿐이라 이 조합이면 빈틈이 없다.
취소된 비행은 **제출 기록을 남기지 않는다.** 예약만 있었고 기록은 착지에서 쓰기 때문이다.

---

## 4. 새 파일 — `BalhwajeomTabletKeywordFlight`

`BalhwajeomPhotoCameraZoom` / `BalhwajeomCameraExitYaw`와 같은 방식. 순수 함수로 빼서 위젯 없이 테스트한다.

```cpp
namespace BalhwajeomTabletKeywordFlight
{
    /** 0..1 eased progress. Exponent above 1 decelerates into the blank. */
    BALHWAJEOM_API float ResolveEaseOutAlpha(float Elapsed, float Duration, float Exponent);

    /** Viewport-space centre of the flying chip at that progress. */
    BALHWAJEOM_API FVector2D ResolveFlightPosition(
        const FVector2D& StartPosition, const FVector2D& EndPosition, float Alpha);

    /** StartScale at the click, 1.0 on arrival. */
    BALHWAJEOM_API float ResolveFlightScale(float StartScale, float Alpha);

    /**
     * Lowest slot that is neither already submitted nor already claimed by a chip in
     * flight, or INDEX_NONE when the sentence has no room left.
     */
    BALHWAJEOM_API int32 ResolveFirstOpenSlot(
        const TArray<int32>& SortedSlotIndices,
        const TArray<int32>& FilledSlotIndices,
        const TArray<int32>& PendingSlotIndices);
}
```

---

## 5. `UBalhwajeomTabletWidget` 변경 지점

### 5.1 튜닝

| 이름 | 기본값 | 의미 |
|---|---|---|
| `KeywordFlightDuration` | `0.25` s | 비행 시간 |
| `KeywordFlightEaseExponent` | `3.0` | 클수록 초반이 급하다. `1.0`은 등속 |
| `KeywordFlightStartScale` | `1.15` | 튀어나가는 느낌. `1.0`이면 끔 |
| `KeywordFlightLayerZOrder` | `110` | 태블릿(100) 위 |

### 5.2 상태

```cpp
struct FKeywordFlight
{
    TWeakObjectPtr<UBalhwajeomTabletWordChip> Chip;
    FName WordID = NAME_None;
    int32 TargetSlotIndex = INDEX_NONE;
    FVector2D StartPosition = FVector2D::ZeroVector;
    FVector2D EndPosition = FVector2D::ZeroVector;
    float Elapsed = 0.0f;
};
TArray<FKeywordFlight> KeywordFlights;

UPROPERTY(Transient)
TObjectPtr<UBalhwajeomTabletKeywordFlightLayer> KeywordFlightLayer;
```

### 5.3 함수

```cpp
bool BeginKeywordFlight(FName WordID, int32 TargetSlotIndex);   // false면 호출자가 즉시 채운다
void TickKeywordFlights(float DeltaTime);
void CancelKeywordFlights(int32 SlotIndex = INDEX_NONE);        // INDEX_NONE = 전부
bool IsSlotPendingKeywordFlight(int32 SlotIndex) const;
UBalhwajeomTabletKeywordFlightLayer* EnsureKeywordFlightLayer();
void ReleaseKeywordFlightLayerIfIdle();
virtual void NativeDestruct() override;
```

`NativeTick()`은 **`PuzzleSuccessStage` 조기 반환보다 앞에서** `TickKeywordFlights()`를 부른다. 다만 성공 전환이 시작되면 어차피 `HidePuzzleWordAndPhotoControls` → 취소가 걸린다.

### 5.4 `HandleWordChipClicked()` 재작성

```text
1. 성공 전환 중이면 무시 (기존 그대로)
2. ActiveBlanksBySlot이 비었으면 무시 (폴더의 일반 단어 목록)
3. ResolveFirstOpenSlot(정렬 슬롯, 제출된 슬롯, 비행 중 슬롯)
4. INDEX_NONE이면 무시 (남은 칸 없음)
5. BeginKeywordFlight(WordID, Slot)
     성공 → 착지 때 채워진다
     실패 → HandleSentenceBlankDropped(...)로 즉시 채운다 (기존 동작)
```

---

## 6. 검증

### 6.1 자동화 — `Balhwajeom.Tablet.KeywordFlight`

`Source/Balhwajeom/Private/Tablet/Test/TabletKeywordFlightTest.cpp` (위젯 불필요)

`ResolveEaseOutAlpha`
1. `t=0 → 0`, `t>=Duration → 1` (정확히 착지)
2. `Duration <= 0`이면 1 (0 나눗셈 없음)
3. 지수 3에서 전반 절반이 이미 절반 이상 진행 — 감속 곡선 확인
4. 단조 증가

`ResolveFlightPosition` / `ResolveFlightScale`
5. `Alpha=0`이면 시작점·`StartScale`, `Alpha=1`이면 도착점·`1.0`
6. `StartScale = 1.0`이면 전 구간 `1.0`

`ResolveFirstOpenSlot` — **요구사항 2의 핵심**
7. 아무것도 안 찼으면 가장 낮은 슬롯
8. 제출된 슬롯은 건너뛴다
9. **비행 중인 슬롯도 건너뛴다** (연타 시 같은 칸에 두 번 들어가지 않음)
10. 제출 + 비행으로 모두 막히면 `INDEX_NONE`
11. 정렬되지 않은 입력에도 최솟값을 고른다

### 6.2 수동 (room3 → 태블릿 → 여동생 폴더 → 가족 사진)

| # | 절차 | 기대 |
|---|---|---|
| T1 | 키워드를 클릭 | 목록 위치에서 빈칸까지 날아간다. 초반이 빠르고 빈칸 앞에서 감속 |
| T2 | 마지막 빈칸을 채우는 클릭 | **칩이 빈칸에 들어간 뒤** 정답/오답 판정이 뜬다 |
| T3 | 키워드 두 개를 빠르게 연타 | 서로 다른 빈칸으로 각각 날아간다 |
| T4 | 키워드를 **드래그**해서 넣기 | 예전처럼 즉시 채워진다 (비행 없음) |
| T5 | 오답 → 퍼즐 리셋 | 날아가던 칩이 남지 않는다 |
| T6 | 비행 중에 팝업을 닫기 | 칩이 화면에 남지 않는다 |
| T7 | 비행 중 같은 칸에 드래그로 드롭 | 비행이 취소되고 드롭한 단어가 남는다 |
| T8 | 진술서 퍼즐에서 T1~T3 반복 | 사진 퍼즐과 동일 |

### 6.3 회귀

`Balhwajeom.Tablet.*`, `Balhwajeom.Camera.*`, `Balhwajeom.Tutorial.*` 전체 실행.

---

## 7. 엣지 케이스

| 상황 | 처리 |
|---|---|
| 칩/빈칸의 지오메트리가 아직 없음 | 비행 없이 즉시 채운다 |
| 남은 빈칸 없음 | 아무 일도 없다 (기존과 동일) |
| 비행 중 오답 리셋 | 전부 취소, 제출 기록 없음 |
| 비행 중 태블릿 닫기 | `NativeDestruct`에서 레이어 제거 |
| 같은 단어를 여러 칸에 | 각각 별개 비행. 단어는 목록에서 사라지지 않으므로 정상 |
| `KeywordFlightDuration = 0` | `ResolveEaseOutAlpha`가 1을 반환 → 첫 틱에 착지 |

---

## 8. 범위 밖

- **사진 증거 슬롯**(`HandlePhotoSlotClicked`)의 비행 연출. 사진은 클릭하면 후보 목록을 여는 다른 흐름이라 같은 처리가 맞지 않는다.
- 빈칸에서 키워드를 **빼낼 때**(`HandleSentenceBlankClicked`)의 역방향 비행. 요청 범위 밖이고, 되돌리기는 즉시가 더 낫다.
- 착지 후 빈칸이 늘어나며 WrapBox가 재배치되는 것. 지금도 동일하게 일어난다.

---

## 9. 작업 목록

| # | 파일 | 내용 |
|---|---|---|
| 1 | `Public/Tablet/BalhwajeomTabletKeywordFlight.h` + `Private/.../cpp` | **신규.** 순수 함수 4개 |
| 2 | `Public/Tablet/BalhwajeomTabletWidget.h` | 레이어 클래스, 튜닝 4개, 상태 2개, 함수 6개 |
| 3 | `Private/Tablet/BalhwajeomTabletWidget.cpp` | 비행 시작/틱/착지/취소, 클릭 경로 재작성 |
| 4 | `Private/Tablet/Test/TabletKeywordFlightTest.cpp` | **신규.** 6.1의 11개 검증 |

블루프린트·`.umap`·DataTable **변경 없음.**
