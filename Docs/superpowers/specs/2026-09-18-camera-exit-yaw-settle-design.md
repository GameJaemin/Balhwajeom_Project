# 카메라 모드 종료 시 캐릭터 방향 이어받기 설계

> 작성일: 2026-09-18
> 대상 레벨: `/Game/Levels/room3`
> 대상 코드: `UBalhwajeomPhotoCameraComponent`, `ABalhwajeomCameraCharacter`
> **상태: 구현 완료 (2026-09-18). 에디터 빌드 성공, 자동화 회귀 0건. 수동 검증(7.2) 미실시.**
>
> **설계와 달라진 점**
> 1. **`bUseControllerRotationYaw`는 나머지 플래그와 함께 미루지 않고 정착 시작 시점에 바로 끈다.**
>    `APawn::FaceRotation()`이 이 플래그가 켜져 있는 동안 매 틱 폰을 컨트롤 회전으로 스냅하기
>    때문이다. 그대로 뒀으면 정착이 매 프레임 덮어써졌고, `bCarryCameraLookYawToExploration`을
>    끈 경우엔 페이드 중간에 **즉시 한 번에 도는** 더 나쁜 결과가 나왔다.
>    `bOrientRotationToMovement` / `bUseControllerDesiredRotation`만 `FinishCameraTransition()`까지 미룬다.
> 2. 테스트를 `…CameraExitYaw.ExplorationYaw` / `…CameraExitYaw.Settle` 두 개로 나눴다.

---

## 0. 결론 먼저

증상은 "종료 연출이 없어서" 생기는 게 아니다. **종료 시점에 세 가지가 같은 프레임에 동시에 바뀌기 때문**이다.

| 바뀌는 것 | 어디서 | 결과 |
|---|---|---|
| ① 컨트롤 회전이 **진입 전 각도로 스냅** | `BalhwajeomPhotoCameraComponent.cpp:915` | 시야가 과거 방향으로 되돌아감 |
| ② 회전 모드가 `bUseControllerRotationYaw` → `bOrientRotationToMovement` | 같은 파일 `:922-928` | 액터 야우를 **이동 방향이 끌고 가기 시작** |
| ③ **W의 의미가 바뀜** | `BalhwajeomCameraCharacter.cpp:274` | 복원된 컨트롤 야우(또는 존 정면) 기준으로 재계산 |

즉 캐릭터는 A 방향을 보며 A로 걷고 있었는데, 종료 순간 **W가 B를 가리키게 되고**, `bOrientRotationToMovement`가 액터를 A→B로 500°/s로 돌린다. 말씀하신 "다른 방향을 보고 있다 / 움직임이 어색하다"가 정확히 이것이다.

**따라서 고칠 것은 두 가지다.**

**결정 1 — 카메라 모드에서 둘러본 야우를 탐색 모드로 이어받는다. (①③ 해결)**
단, 진입 시 적용된 **패럴랙스 보정량만 빼고** 이어받는다. 그래야 코드 주석이 경고하는 드리프트 회귀가 재발하지 않는다.

**결정 2 — 남은 회전은 페이드인 구간에 걸쳐 `bOrientRotationToMovement`를 대신 흉내 내어 끝낸다. (② 해결)**
회전이 "페이드가 끝난 뒤 시작"되는 게 아니라 **"페이드가 끝나는 시점에 이미 완료"** 되도록 만든다.

결정 1만으로 일반 구간의 위화감은 사라진다. 결정 2는 **고정 카메라 존(`ABalhwajeomFixedCameraZone`) 안**처럼 방향 불연속이 구조적으로 불가피한 경우를 덮는 마감이다.

---

## 1. 현재 동작 정리

### 1.1 탐색 모드 (3인칭)

`ABalhwajeomCameraCharacter` 생성자 `:32-37`:

```text
bUseControllerRotationYaw     = false
bOrientRotationToMovement     = true
RotationRate                  = (0, 500, 0)
```

→ 액터 야우는 **이동 입력 방향을 500°/s로 따라간다.** 이동 방향은
`ActiveCameraZone`이 있으면 **존의 평면 정면**, 없으면 **컨트롤 야우 기준**이다.

### 1.2 카메라 모드 (1인칭)

`EnterCameraMode()` `:800-850`:

```text
bOrientRotationToMovement     = false
bUseControllerDesiredRotation = false
bUseControllerRotationYaw     = true      ← 액터 야우 = 컨트롤 야우 (보간 없음)

SavedExplorationControlRotation = 진입 직전 컨트롤 회전
SetControlRotation(화면 중앙 물체를 겨냥한 회전)   ← 패럴랙스 보정 D 만큼 틀어짐
```

이동은 존을 무시하고 항상 컨트롤 야우 기준이다 (`BalhwajeomCameraCharacter.cpp:265`의 `bFirstPersonCameraMode` 분기).

### 1.3 전환 타임라인

`ToggleCameraMode()`는 `CameraTransitionDuration`(기본 0.5초)을 반으로 나눠 쓴다.

```text
t=0          RMB/TAB         페이드 아웃 시작 (검정으로)
t=0.25       SwitchCameraAtFadeOut()
               └ ExitCameraMode()      ← ①②③ 이 여기서 한꺼번에 발생
             페이드 인 시작 (검정에서 밝게)
t=0.50       FinishCameraTransition()  ← bIsCameraTransitioning = false
```

**문제의 회전은 t=0.25에 시작해서 화면이 밝아지는 내내 진행된다.** 180° 회전이면 500°/s로 0.36초가 걸려 **페이드가 끝난 뒤(t=0.50~0.61)까지 눈에 그대로 보인다.** 게다가 이동 방향까지 바뀌었으므로 회전이 끝난 뒤에도 "엉뚱한 데로 걸어가는" 상태가 계속된다.

---

## 2. 기각한 대안

| 대안 | 기각 이유 |
|---|---|
| `RotationRate`를 올린다 | 회전이 빨라질 뿐 **W의 방향이 바뀌는 문제(①③)는 그대로**다. 탐색 모드 전체의 조작감도 같이 바뀐다. |
| `SetControlRotation(Saved…)` 삭제 | 진입 패럴랙스 보정 D가 RMB를 누를 때마다 누적된다. 코드 주석 `:911-914`가 경고하는 바로 그 드리프트 버그의 회귀. |
| 전환 중 이동 입력 차단 | W를 누르고 있는 상황이 정확히 문제 상황이다. 0.25초 멈칫하는 쪽이 더 나쁘다. |
| `CameraTransitionDuration`을 늘려 가린다 | 응답성만 깎이고 원인은 그대로다. |

---

## 3. 설계

### 3.1 결정 1 — 진입 보정량을 빼고 야우 이어받기

진입 시 적용한 보정량을 기억해 둔다.

```text
진입:  D = Unwind(정렬후_야우 − 진입직전_야우)        // EntryAlignmentYawDelta
종료:  탐색야우 = Unwind(현재_카메라_야우 − D)
       SetControlRotation( (저장된Pitch, 탐색야우, 저장된Roll) )
```

**드리프트가 재발할 수 없는 이유**: 카메라 모드에서 마우스를 전혀 안 움직이면
`현재_카메라_야우 == 저장야우 + D` 이므로 `탐색야우 == 저장야우`로 **정확히 복원**된다.
둘러본 만큼만 그대로 따라 나온다.

**피치·롤은 저장값을 그대로 쓴다.** 카메라 모드 피치는 자유 시점이고 3인칭 피치는 스프링암 각도라, 바닥을 보다 카메라를 내리면 3인칭 카메라가 바닥을 향하게 된다.

**안전판**: `bCarryCameraLookYawToExploration`(기본 `true`)를 끄면 기존 동작(저장값 전체 복원)으로 즉시 되돌아간다. 마감 직전 변경이므로 팀이 이 연출을 싫어하면 bool 하나로 회귀할 수 있어야 한다. 이때도 결정 2의 부드러운 회전은 계속 동작한다.

### 3.2 결정 2 — 페이드인 구간에 걸친 야우 정착(settle)

`ExitCameraMode()`에서 **이동 회전 플래그 복원을 즉시 하지 않고 `FinishCameraTransition()`까지 미룬다.** 그 사이 `bOrientRotationToMovement`도 `bUseControllerRotationYaw`도 꺼져 있으므로 **아무도 액터 야우를 건드리지 않는다.** 그 빈 구간을 컴포넌트가 직접 채운다.

```text
t=0.25  ExitCameraMode()
          ├ 컨트롤 회전 = 결정 1의 탐색야우
          ├ 회전 플래그는 아직 카메라 모드 상태(둘 다 false)로 유지
          └ 정착 시작   ExplorationYawSettleRemaining = CameraTransitionDuration * 0.5

t=0.25~0.50  매 틱:
          목표야우 = 이동 가속도의 야우            ← CMC가 쓸 값과 동일
          (가속도가 없으면 회전하지 않음 — 제자리에서 도는 걸 막는다)
          액터 야우 = StepSettleYaw(현재, 목표, dt, 남은시간, 이징)

t=0.50  FinishCameraTransition()
          ├ bOrientRotationToMovement / bUseControllerRotationYaw 복원
          └ bIsCameraTransitioning = false
```

**핵심은 목표 야우를 `GetCharacterMovement()->GetCurrentAcceleration()`에서 가져온다는 점이다.**
이는 `bOrientRotationToMovement`가 내부적으로 쓰는 값과 같다. 따라서 t=0.50에 플래그를 되돌려줄 때 **CMC가 이어받을 각도와 정착이 끝낸 각도가 일치**하고, 두 번째 불연속이 생기지 않는다. 가속도가 거의 0이면 CMC도 회전하지 않으므로 정착도 현재 야우를 유지한다.

정착 구간을 페이드인 길이에 **묶는** 이유: 더 길면 화면이 밝아진 뒤에도 회전이 보이고, 더 짧으면 검은 화면을 낭비한다.

### 3.3 이징 — 어두울 때 많이 돌린다

페이드인 구간은 "검정 → 밝음"이다. 회전을 **앞쪽(가장 어두울 때)에 몰아야** 눈에 덜 띈다.

```text
Alpha = clamp(dt / 남은시간, 0, 1)          // 등속이면 그대로 쓰면 됨
Eased = Alpha ^ EaseExponent                // 지수 < 1 → 초반에 더 크게 문다
새야우 = Unwind(현재야우 + Unwind(목표 − 현재) * Eased)
```

`EaseExponent = 0.6` 기본. `1.0`이면 등속이 된다. 남은시간이 dt 이하가 되면 목표값을 그대로 반환해 **정확히 착지**시킨다(누적 오차 없음).

---

## 4. 새 파일 — `BalhwajeomCameraExitYaw`

`BalhwajeomPhotoCameraZoom` / `BalhwajeomCameraFocusModel`과 같은 방식이다. 순수 함수로 빼서 **월드도 폰도 없이 테스트**한다.

`Source/Balhwajeom/CameraSystem/BalhwajeomCameraExitYaw.h`

```cpp
#pragma once

#include "CoreMinimal.h"


/**
 * Yaw handoff between photo camera mode and exploration.
 *
 * Entry aims the eye camera at the world point under the outgoing screen centre, which
 * adds a parallax delta to the control yaw. Carrying the camera-mode yaw straight back
 * out would re-apply that delta on every RMB press, so the exit subtracts it: a trip
 * with no look input lands on the exact yaw it started from, and anything the player
 * actually looked around by is kept.
 *
 * The settle step replaces bOrientRotationToMovement for the length of the fade-in, so
 * the turn is finished by the time the screen clears instead of starting there.
 */
namespace BalhwajeomCameraExitYaw
{
    /** Camera-mode yaw minus the alignment the entry applied, unwound to (-180, 180]. */
    BALHWAJEOM_API float ResolveExplorationYaw(
        float CameraModeYaw,
        float EntryAlignmentYawDelta);

    /**
     * One frame of the settle. Lands exactly on TargetYaw when the remaining time runs
     * out, so the handoff never leaves a residue. EaseExponent below 1 takes a larger
     * bite early, which puts most of the turn under the darkest part of the fade; 1.0
     * is constant speed. A non-positive remaining time snaps to the target.
     */
    BALHWAJEOM_API float StepSettleYaw(
        float CurrentYaw,
        float TargetYaw,
        float DeltaSeconds,
        float RemainingSeconds,
        float EaseExponent);
}
```

`.cpp`는 위 3.1·3.3 수식 그대로. `FMath::UnwindDegrees`로 최단 방향 회전을 보장한다(179°와 -181°를 같게 취급).

---

## 5. `UBalhwajeomPhotoCameraComponent` 변경 지점

### 5.1 헤더 추가분

```cpp
// 튜닝
/** Keeps the yaw the player looked around by when the camera comes down. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Exit")
bool bCarryCameraLookYawToExploration = true;

/** Below 1 front-loads the settle into the darkest part of the fade; 1.0 is constant speed. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Exit",
    meta = (ClampMin = "0.1", ClampMax = "1.0"))
float ExplorationYawSettleEaseExponent = 0.6f;

// 상태
float EntryAlignmentYawDelta = 0.0f;
bool  bIsSettlingExplorationYaw = false;
float ExplorationYawSettleRemaining = 0.0f;

// 동작
void RestoreExplorationMovementRotation();
void UpdateExplorationYawSettle(float DeltaTime);
void CancelExplorationYawSettle();
bool ResolveExplorationSettleTargetYaw(float& OutYaw) const;

virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
```

### 5.2 `EnterCameraMode()` — `:847`

`SetControlRotation(...)` 호출을 지역 변수로 받아서 델타를 기록한다.

```cpp
const FRotator AlignedRotation = (!bFoundCenterTarget || PhotoViewDirection.IsNearlyZero())
    ? OutgoingViewRotation
    : PhotoViewDirection.Rotation();
EntryAlignmentYawDelta = FMath::UnwindDegrees(
    AlignedRotation.Yaw - SavedExplorationControlRotation.Yaw);
PlayerController->SetControlRotation(AlignedRotation);
```

함수 첫머리에 `CancelExplorationYawSettle();`을 추가한다(방어용). `PlayerController`가 없으면 `EntryAlignmentYawDelta = 0.0f`.

### 5.3 `ExitCameraMode()` — `:913-929`

```cpp
if (APlayerController* PlayerController = ...; PlayerController && bHasSavedExplorationControlRotation)
{
    const float ExplorationYaw = bCarryCameraLookYawToExploration
        ? BalhwajeomCameraExitYaw::ResolveExplorationYaw(
              PlayerController->GetControlRotation().Yaw, EntryAlignmentYawDelta)
        : SavedExplorationControlRotation.Yaw;

    PlayerController->SetControlRotation(FRotator(
        SavedExplorationControlRotation.Pitch,
        ExplorationYaw,
        SavedExplorationControlRotation.Roll));
}
bHasSavedExplorationControlRotation = false;
EntryAlignmentYawDelta = 0.0f;
```

그리고 **`:922-929`의 회전 플래그 복원 블록을 `RestoreExplorationMovementRotation()`으로 추출**해 여기서는 호출하지 않는다. 대신 정착을 시작한다.

```cpp
bIsSettlingExplorationYaw = (Cast<ACharacter>(GetOwner()) != nullptr);
ExplorationYawSettleRemaining = CameraTransitionDuration * 0.5f;
```

`bHasSavedFirstPersonMovementMode`는 **여기서 지우지 않는다.** 복원 함수가 소비한다.

### 5.4 틱 유지 — `:949`

```cpp
SetComponentTickEnabled(bIsSettlingExplorationYaw);   // was: false
```

`TickComponent()` `:372`는 이미 `bIsInCameraMode`로 포커스 작업을 막고 있으므로 분기만 추가하면 된다.

```cpp
if (bIsSettlingExplorationYaw)
{
    UpdateExplorationYawSettle(DeltaTime);
}
```

### 5.5 `UpdateExplorationYawSettle()`

```cpp
ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
if (!OwnerCharacter) { CancelExplorationYawSettle(); return; }

float TargetYaw = 0.0f;
if (ResolveExplorationSettleTargetYaw(TargetYaw))   // 가속도가 있을 때만 true
{
    const FRotator Current = OwnerCharacter->GetActorRotation();
    const float NewYaw = BalhwajeomCameraExitYaw::StepSettleYaw(
        Current.Yaw, TargetYaw, DeltaTime,
        ExplorationYawSettleRemaining, ExplorationYawSettleEaseExponent);
    OwnerCharacter->SetActorRotation(FRotator(Current.Pitch, NewYaw, Current.Roll));
}

ExplorationYawSettleRemaining -= DeltaTime;
```

`ResolveExplorationSettleTargetYaw()`는 CMC와 같은 기준을 쓴다.

```cpp
const FVector Acceleration = Movement->GetCurrentAcceleration();
if (Acceleration.SizeSquared2D() < KINDA_SMALL_NUMBER) { return false; }  // CMC와 동일 판정
OutYaw = Acceleration.Rotation().Yaw;
return true;
```

정착 중에도 `MoveForward`/`MoveRight`는 이미 **복원된 컨트롤 야우(또는 존 정면)** 로 계산되므로, 가속도는 곧 "종료 후에 W가 밀어낼 실제 세계 방향"이다.

### 5.6 종료·취소

```cpp
void FinishCameraTransition()          // :1051
{
    RestoreExplorationMovementRotation();
    bIsSettlingExplorationYaw = false;
    ExplorationYawSettleRemaining = 0.0f;
    if (!bIsInCameraMode) { SetComponentTickEnabled(false); }
    bIsCameraTransitioning = false;
    OnCameraTransitionFinished.Broadcast();
}

void CancelExplorationYawSettle()      // 정착 중이면 즉시 플래그 복원
{
    if (!bIsSettlingExplorationYaw) { return; }
    RestoreExplorationMovementRotation();
    bIsSettlingExplorationYaw = false;
    ExplorationYawSettleRemaining = 0.0f;
}
```

`RestoreExplorationMovementRotation()`은 `bHasSavedFirstPersonMovementMode`가 켜져 있을 때만 동작하고, 끝나면 내린다. **두 번 불려도 안전해야 한다.**

취소 경로: `EnterCameraMode()` 첫머리, 새로 추가하는 `EndPlay()`.
`ToggleCameraMode()`는 `bIsCameraTransitioning`으로 재진입을 이미 막으므로 별도 처리가 필요 없다.

---

## 6. 튜닝 파라미터

| 이름 | 기본값 | 의미 |
|---|---|---|
| `bCarryCameraLookYawToExploration` | `true` | 끄면 종료 각도가 기존 동작(진입 전 각도)으로 회귀 |
| `ExplorationYawSettleEaseExponent` | `0.6` | 낮을수록 어두울 때 많이 돈다. `1.0`은 등속 |
| `CameraTransitionDuration` (기존) | `0.5` | 정착 길이는 이 값의 절반으로 따라간다 |

---

## 7. 검증

### 7.1 자동화 — `Balhwajeom.Camera.CameraExitYaw`

`Source/Balhwajeom/Private/CameraSystem/Test/CameraExitYawTest.cpp` (월드 불필요)

`ResolveExplorationYaw`
1. **드리프트 회귀 방지 (가장 중요)** — 둘러보지 않았을 때(`CameraModeYaw == Saved + D`) 결과가 `Saved`와 정확히 일치한다.
2. 둘러본 각도가 그대로 더해져 나온다.
3. 180° 경계를 넘어도 최단 방향으로 언와인드된다.
4. `D == 0`이면 입력 야우를 그대로 돌려준다.

`StepSettleYaw`
5. `RemainingSeconds <= DeltaSeconds`면 정확히 `TargetYaw`를 반환한다 (착지 보장).
6. `RemainingSeconds <= 0`이어도 목표로 스냅한다 (0 나눗셈 없음).
7. 이징 지수 `< 1`일 때 첫 스텝이 등속(`1.0`)보다 크다.
8. 반복 호출이 단조적으로 목표에 수렴하고 넘어가지 않는다(오버슈트 없음).
9. 350° → 10° 같은 경우 **+20°로** 움직인다(-340° 아님).

### 7.2 수동 (room3, PIE)

| # | 절차 | 기대 |
|---|---|---|
| M1 | 카메라 모드에서 한쪽을 보며 **W를 계속 누른 채** 종료 | 진행 방향·바라보는 방향이 그대로 유지된다. 회전이 눈에 띄지 않는다 |
| M2 | 제자리에서(입력 없이) 둘러본 뒤 종료 | 캐릭터가 제자리에서 돌지 않는다. 3인칭 시야만 보던 쪽으로 이어진다 |
| M3 | **RMB로 켜고 끄기를 10회 반복**(마우스 고정) | 3인칭 시야 각도가 처음과 같다 — **드리프트 0** |
| M4 | 고정 카메라 존 안에서 W를 누른 채 종료 | 존 정면으로 바뀌는 회전이 페이드 안에서 매끄럽게 끝난다 |
| M5 | 촬영 → 자동 종료(`bExitCameraWhenNoCapturableTargetsRemain`) | 카드가 날아가는 동안 회전이 끝나 있다 |
| M6 | 종료 전환 중 TAB(태블릿)을 연다 | 캐릭터가 회전 도중에 멈춘 채 방치되지 않는다 |
| M7 | 종료 직후 즉시 방향 전환(A/D) | 정착이 CMC로 넘어가는 지점에 끊김이 없다 |
| M8 | `bCarryCameraLookYawToExploration`을 **끄고** M1 반복 | 기존처럼 진입 전 각도로 돌아가되, 회전이 **즉시 스냅되지 않고** 페이드 안에서 끝난다 |

### 7.3 회귀

- `Balhwajeom.Camera.*`, `Balhwajeom.Tutorial.*` 전체 실행
- 튜토리얼 `Tutorial.Trigger.PhotoCaptureCompleted`는 `ExitCameraMode()` 안에서 붙는다 — 정착 도입으로 타이밍이 밀리지 않는지 확인 (`:963-975`)
- `OnCameraModeExited` → `ActiveCameraZone->ActivateCamera()` 순서 유지 확인 (`:1002`)

---

## 8. 엣지 케이스

| 상황 | 처리 |
|---|---|
| 소유자가 `ACharacter`가 아님 | 정착을 시작하지 않고 `ExitCameraMode()`에서 플래그를 즉시 복원 |
| 전환 중 폰 파괴 / 레벨 전환 | `EndPlay()`에서 `CancelExplorationYawSettle()` |
| 존이 `bResetYawOnEnter`로 야우를 리셋 | 목표를 매 틱 재계산하므로 자동으로 따라간다 |
| 정착 중 플레이어가 W를 놓음 | 가속도가 0 → 그 각도에서 멈춘다. CMC도 같은 판정이라 이어받을 때 어긋나지 않는다 |
| 정착 중 방향키를 바꿈 | 목표가 매 틱 갱신되어 새 방향으로 향한다 |
| `CameraTransitionDuration`이 매우 작음 | 첫 틱에서 `RemainingSeconds <= DeltaSeconds` → 목표로 착지 |
| 카메라 모드 재진입이 정착 중 발생 | `ToggleCameraMode()`가 `bIsCameraTransitioning`으로 이미 차단 |

---

## 9. 범위 밖

- **진입 방향 불연속**: 진입 시에도 `bUseControllerRotationYaw = true`가 액터 야우를 즉시 스냅하고, W 방향이 패럴랙스 보정 `D`만큼 틀어진다. 다만 진입은 완전한 암전 중에 일어나고 1인칭에서는 몸이 숨겨지므로(`SetOwnerNoSee(true)`) 보이지 않는다. `D`도 보통 몇 도 수준이라 이번 작업에 포함하지 않는다.
- **실제 카메라를 내리는 몽타주**: 현재 "종료 연출"은 페이드뿐이고 별도 애니메이션 에셋이 없다. 이 설계는 페이드 길이에 맞춰 회전을 끝내므로, 나중에 몽타주가 생기면 `ExplorationYawSettleRemaining`의 출처만 그 길이로 바꾸면 된다.
- 3인칭 피치를 카메라 모드 피치에서 이어받는 것 (3.1의 결정대로 제외).

---

## 10. 작업 목록

| # | 파일 | 내용 |
|---|---|---|
| 1 | `CameraSystem/BalhwajeomCameraExitYaw.h/.cpp` | **신규.** 순수 함수 2개 |
| 2 | `CameraSystem/BalhwajeomPhotoCameraComponent.h` | 튜닝 2개, 상태 3개, 함수 4개, `EndPlay` 오버라이드 |
| 3 | `CameraSystem/BalhwajeomPhotoCameraComponent.cpp` | 진입 델타 기록 / 종료 야우 계산 / 플래그 복원 지연 / 틱 분기 / 취소 경로 |
| 4 | `Private/CameraSystem/Test/CameraExitYawTest.cpp` | **신규.** 7.1의 9개 검증 |

`ABalhwajeomCameraCharacter`, 블루프린트, `.umap`, DataTable **변경 없음.**
`BalhwajeomPhotoCameraComponent.cpp`에 필요한 헤더(`GameFramework/CharacterMovementComponent.h` 등)는 이미 전부 포함되어 있다.
