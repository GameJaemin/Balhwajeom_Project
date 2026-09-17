# 3D 아이템 인스펙터 입력 포커스 복구 설계

작성 기준일: 2026-09-17
대상: `ItemInspector` 플러그인 + `Balhwajeom` 상호작용 모듈 / Unreal Engine 5.7

## 1. 증상

고데기 같은 `JMInspectableComponent` 오브젝트를 F로 열면 3D 회전 뷰(`WBP_JMItemInspection`)가 뜬다.
이 상태에서 **화면을 한 번 우클릭하면 F / Esc로 닫는 기능이 완전히 죽는다.**
마우스 좌드래그 회전과 휠 줌은 그대로 동작하기 때문에 "UI는 살아있는데 키보드만 죽은" 상태로 보인다.
현재 인스펙터를 닫을 방법은 (닫기 버튼이 노출돼 있지 않은 레이아웃에서는) 사실상 없다.

## 2. 원인

키보드가 아니라 **Slate 키보드 포커스**가 우클릭 한 번에 게임 뷰포트로 넘어가는 것이 원인이다. 체인은 다음과 같다.

1. 인스펙터는 `FInputModeGameAndUI` + `SetWidgetToFocus(위젯)`로 열린다.
   [JMItemInspectionSubsystem.cpp:380](../../Plugins/ItemInspector/Source/ItemInspectorRuntime/Private/ItemInspection/JMItemInspectionSubsystem.cpp:380), [:393](../../Plugins/ItemInspector/Source/ItemInspectorRuntime/Private/ItemInspection/JMItemInspectionSubsystem.cpp:393)
2. 엔진의 `FInputModeGameAndUI::ApplyInputMode`는 뷰포트 캡처 모드를 **`EMouseCaptureMode::CaptureDuringMouseDown`** 으로 강제한다.
   `Engine/Source/Runtime/Engine/Private/PlayerController.cpp:6288`
3. 인스펙터 위젯은 **좌클릭만** 소비한다. 우클릭/휠클릭은 `Super`로 흘려보내 `Unhandled`가 된다.
   [JMItemInspectionWidgetBase.cpp:88](../../Plugins/ItemInspector/Source/ItemInspectorRuntime/Private/ItemInspection/JMItemInspectionWidgetBase.cpp:88)
4. 소비되지 않은 마우스 다운은 상위 `SViewport`까지 버블링되고, `FSceneViewport::OnMouseButtonDown`은 캡처 모드가 `CaptureDuringMouseDown`이므로 `AcquireFocusAndCapture(..., EFocusCause::Mouse)`를 실행한다. 그 안에서 **`SetUserFocus(ViewportWidget)`** 이 호출된다.
   `Engine/Source/Runtime/Engine/Private/Slate/SceneViewport.cpp:552-581`, `:591-601`
5. 그 순간 인스펙터 위젯은 키보드 포커스를 잃는다. `NativeOnKeyDown`이 아예 호출되지 않으므로 F / Esc 처리 코드가 죽는다.
   [JMItemInspectionWidgetBase.cpp:74](../../Plugins/ItemInspector/Source/ItemInspectorRuntime/Private/ItemInspection/JMItemInspectionWidgetBase.cpp:74)
6. 포커스를 되찾아주는 코드가 어디에도 없다. 마우스를 위젯 위에서 다시 움직이거나 좌드래그를 해도 좌클릭은 `CaptureMouse`만 할 뿐 포커스를 다시 잡지 않는다.

여기서 끝나지 않고, 포커스가 넘어간 뒤의 **대체 경로도 전부 막혀 있어서** "아무 반응 없음"이 된다.

- F는 게임 입력(`IA_Interact`)으로 들어가지만 `ResolveInspectable`이 `IsOpen(Pawn)`에서 즉시 `nullptr`을 반환한다 → 무반응.
  [ItemInspectionIntegration.cpp:31](../../Source/Balhwajeom/Private/Interaction/ItemInspectionIntegration.cpp:31), [PlayerInteractionComponent.cpp:457](../../Source/Balhwajeom/Private/Interaction/PlayerInteractionComponent.cpp:457)
- Esc는 레거시 액션 `ExitCameraMode` → `RequestExitCameraMode`로 가지만 카메라 모드가 아니므로 무반응.
  [Balhwajeom.cpp:36](../../Source/Balhwajeom/Balhwajeom.cpp:36), [BalhwajeomCameraCharacter.cpp:252](../../Source/Balhwajeom/CameraSystem/BalhwajeomCameraCharacter.cpp:252)
- 우클릭 자체는 레거시 액션 `CameraMode`에 묶여 있지만 `ToggleCameraMode`가 `IsOpen`으로 막혀 있어 아무 일도 일어나지 않는다. **즉 우클릭은 "보이는 효과는 0, 포커스만 뺏는" 입력이다.**
  [Balhwajeom.cpp:35](../../Source/Balhwajeom/Balhwajeom.cpp:35), [BalhwajeomPhotoCameraComponent.cpp:452](../../Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp:452)

부수 증상: `FInputModeGameAndUI`의 `bHideCursorDuringCapture` 기본값이 `true`라서, 우클릭을 누르고 있는 동안 커서가 사라지고 고정밀(raw) 마우스 모드로 전환된다. 이것도 같은 경로에서 생긴다.

### 같은 버그의 기존 해결 사례

프로젝트 내 상호작용 모달은 **이미 같은 문제를 겪고 같은 방식으로 고쳐져 있다.**
[BalhwajeomInteractionModalWidget.cpp:104](../../Source/Balhwajeom/Private/UI/BalhwajeomInteractionModalWidget.cpp:104) — `NativeOnPreviewMouseButtonDown`에서 모든 클릭을 삼키고 `SetKeyboardFocus()`를 다시 건다. 주석에도 "첫 F가 닫기 대신 포커스 복구에 소모된다"고 적혀 있다.
인스펙터 위젯에는 이 처리가 빠져 있다. 즉 이번 건은 **신규 설계가 아니라 누락된 방어 처리의 이식**이다.

## 3. 설계 목표

1. 인스펙터가 떠 있는 동안에는 **어떤 마우스 버튼을 눌러도** 키보드 포커스가 위젯에 남는다.
2. 그럼에도 포커스를 잃는 경로(알트탭 복귀, 콘솔, 외부 코드의 `SetInputMode` 등)가 생기면 **스스로 복구**한다.
3. 위 두 개가 모두 실패해도 **F는 항상 인스펙터를 닫는다** (막다른 상태가 없다).
4. 기존 좌드래그 회전 / 휠 줌 / 전환 애니메이션 동작은 변경하지 않는다.

## 4. 변경 설계

### 계층 1 — 위젯이 모든 포인터 다운을 소유한다 (핵심 수정)

대상: `UJMItemInspectionWidgetBase`

- `NativeOnPreviewMouseButtonDown` 오버라이드를 추가한다. 터널링 단계라 어떤 자식보다 먼저 실행된다.
  - 항상 `SetKeyboardFocus()`를 호출해 포커스를 되잡는다.
  - **좌클릭이면 `FReply::Unhandled()`** 를 반환한다. 기존 버블 경로(`NativeOnMouseButtonDown`의 드래그 시작 + `CaptureMouse`)를 그대로 살리기 위해서다.
  - **그 외 버튼(우/휠/썸)은 `FReply::Handled()`** 로 소비한다. `SViewport`까지 내려가지 않으므로 `AcquireFocusAndCapture`가 실행되지 않는다.
- `NativeOnMouseButtonUp`도 좌클릭 외 버튼을 `Handled`로 소비해 다운/업 쌍을 맞춘다. (`CameraMode`의 `IE_Released`가 게임 입력으로 새는 것을 막는다.)
- `NativeOnKeyDown`, 드래그/줌 로직은 그대로 둔다.

```cpp
FReply UJMItemInspectionWidgetBase::NativeOnPreviewMouseButtonDown(
    const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // GameAndUI는 뷰포트 캡처 모드를 CaptureDuringMouseDown으로 강제한다. 여기서 소비하지 않은
    // 클릭은 SViewport까지 내려가 AcquireFocusAndCapture로 키보드 포커스를 가져가고, 그때부터
    // F/Esc가 NativeOnKeyDown에 도달하지 못한다.
    SetKeyboardFocus();

    // 좌클릭은 버블 단계의 드래그 시작이 처리한다.
    return InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
        ? FReply::Unhandled()
        : FReply::Handled();
}
```

전제 조건: 위젯 루트가 전체 화면에서 히트 테스트 대상이어야 터널 경로에 들어온다.
`NativeConstruct`의 `SetVisibility(ESlateVisibility::Visible)`([:46](../../Plugins/ItemInspector/Source/ItemInspectorRuntime/Private/ItemInspection/JMItemInspectionWidgetBase.cpp:46))로 충족되지만, `WBP_JMItemInspection`에서 루트 가시성이 `SelfHitTestInvisible` / `HitTestInvisible`로 덮어써지지 않았는지 작업 시 확인한다.

추가로 `CreateInspectionWidget`의 입력 모드에 `SetHideCursorDuringCapture(false)`를 넣어, 캡처가 발생하더라도 커서가 깜빡이지 않게 한다.

### 계층 2 — 세션 헬스 틱의 포커스 워치독

대상: `UJMItemInspectionSubsystem::TickSessionHealth` ([:1274](../../Plugins/ItemInspector/Source/ItemInspectorRuntime/Private/ItemInspection/JMItemInspectionSubsystem.cpp:1274))

이미 매 프레임 세션 유효성을 점검하는 코어 티커가 있으므로 여기에 한 조건만 더 얹는다.

- 조건: 인스펙터가 열려 있고 / 위젯이 뷰포트에 있고 / 위젯과 그 자식 모두 유저 포커스가 없고 / **현재 포커스 위젯이 게임 뷰포트 위젯일 때** → `CurrentWidget->SetKeyboardFocus()`.
- 마지막 조건이 중요하다. 콘솔(`~`), 에디터 창, 다른 UMG 모달로 포커스가 간 경우까지 빼앗지 않기 위해 **뷰포트가 포커스를 가진 경우에만** 복구한다.
- 판정 자체는 Slate 없이 테스트할 수 있도록 순수 함수로 분리한다.

```cpp
namespace JMItemInspectionFocus
{
    bool ShouldRestoreWidgetFocus(
        bool bInspectionOpen, bool bWidgetInViewport,
        bool bWidgetOwnsFocus, bool bGameViewportOwnsFocus);
}
```

### 계층 3 — F 폴백 (최후의 탈출구)

대상: `BalhwajeomItemInspection` / `UPlayerInteractionComponent`

계층 1·2가 모두 실패해도 플레이어가 갇히지 않게 한다.

- `ItemInspectionIntegration`에 `RequestClose(AActor* PlayerActor)`를 추가한다 (내부적으로 `UJMItemInspectionSubsystem::CloseInspection(EJMItemInspectionCloseReason::User)`).
- `UPlayerInteractionComponent::RequestInspect` 진입부에서 인스펙션이 열려 있으면 새 조사를 시도하는 대신 **닫기를 요청하고 `true`를 반환**한다.
- **전환 중에는 닫지 않는다.** 플레이어 폰 `BP_OrbitViewCharacter_Legacy`에는 C++ 부모(`ABalhwajeomCameraCharacter`)가 만드는 `PlayerInteractionComponent`와 BP가 추가한 `BPC_PlayerInteraction`이 **둘 다** 붙어 있어, F 한 번에 `RequestInspect`가 같은 프레임에 두 번 호출된다. 첫 호출이 연 인스펙터를 두 번째 호출이 닫으면 입장 전환(불투명도 0 상태)에서 즉시 종료되어 **모델이 한 프레임도 그려지지 않는다.** 따라서 `State == Inspecting`일 때만 닫는다.
- 이중 닫힘 위험 없음: 위젯이 포커스를 가진 정상 상태에서는 `NativeOnKeyDown`이 F를 `Handled`로 소비하므로 게임 입력까지 내려가지 않는다. 또한 서브시스템은 `bCloseInProgress` / `State` 가드를 이미 갖고 있다.
- 정책도 순수 함수로 분리해 테스트한다.

```cpp
enum class EBalhwajeomInteractAction : uint8 { None, CloseInspection, Interact };
EBalhwajeomInteractAction ResolveInteractAction(
    bool bInspectionOpen, bool bInspectionInteractive, bool bOtherModalOpen);
```

> 폰에 상호작용 컴포넌트가 두 개 붙어 있는 것 자체는 별개의 선행 버그다. 트레이스/프롬프트 갱신도 매 틱 두 번 돌고, 멱등하지 않은 상호작용은 한 번의 F에 두 번 실행된다. 이번 수정은 그 상태에서도 안전하게 동작하도록 만든 것이며, 중복 컴포넌트 정리는 콘텐츠(BP) 쪽 결정이 필요하다.

### 채택하지 않은 대안

| 대안 | 판단 |
| --- | --- |
| `FInputModeUIOnly`로 전환 | 포커스 탈취는 사라지지만 인스펙터가 게임 입력을 완전히 차단하는 모달이 된다. 현재 인스펙터는 `bBlockPlayerInput`으로 이동/시야만 막는 준모달이고 전환 연출 중 게임 틱을 전제로 한다. 동작 변화 폭이 커서 보류. |
| `IInputProcessor`로 Slate 단계에서 키를 선점 (`FCapturePhotoDismissInputProcessor` 방식) | 가장 강력하지만 재사용 플러그인(`ItemInspector`)이 프로젝트 입력 정책을 떠안게 된다. 계층 1+2로 충분. |
| 우클릭에 팬/추가 회전 기능 부여 | 이번 버그 범위 밖. 지금은 "소비하고 무시"로 고정한다. |

## 5. 변경 파일

| 파일 | 내용 | 계층 |
| --- | --- | --- |
| `Plugins/ItemInspector/.../Public/ItemInspection/JMItemInspectionWidgetBase.h` | `NativeOnPreviewMouseButtonDown` 선언 | 1 |
| `Plugins/ItemInspector/.../Private/ItemInspection/JMItemInspectionWidgetBase.cpp` | 프리뷰 다운 구현, 비좌클릭 업 소비 | 1 |
| `Plugins/ItemInspector/.../Private/ItemInspection/JMItemInspectionSubsystem.cpp` | `SetHideCursorDuringCapture(false)`, 포커스 워치독 | 1·2 |
| `Plugins/ItemInspector/.../Public/ItemInspection/JMItemInspectionSubsystem.h` | 워치독 판정 함수 선언 | 2 |
| `Plugins/ItemInspector/Source/ItemInspectorTests/.../JMItemInspectionWidgetInputTests.cpp` | 회귀 테스트 추가 | 4 |
| `Source/Balhwajeom/Public\|Private/Interaction/ItemInspectionIntegration.*` | `RequestClose`, `ResolveInteractAction` | 3 |
| `Source/Balhwajeom/Private/Interaction/PlayerInteractionComponent.cpp` | F 폴백 분기 | 3 |
| `Source/Balhwajeom/Private/Interaction/Test/` | 폴백 정책 테스트 | 4 |
| `Docs/Systems/Item-Inspection.md` | 입력/포커스 규칙 한 단락 추가 | 문서 |

> `ItemInspector`는 `Reuse_Plugin`(커밋 `7b52cb1`)에서 가져온 재사용 플러그인이다. 계층 1·2는 프로젝트 고유 버그가 아니라 플러그인 일반 결함이므로 플러그인 쪽에 넣고, 나중에 업스트림에 역반영할 수 있도록 커밋을 분리한다.

## 6. 테스트 계획

자동화 (`ItemInspectorTests`, 기존 `JM.ItemInspector.WidgetInput.*` 네이밍 유지):

- `RightClickIsConsumedByPreview` — 우클릭 프리뷰 다운이 `Handled`.
- `LeftClickFallsThroughToDrag` — 좌클릭 프리뷰 다운이 `Unhandled`이고, 이어지는 버블 다운이 드래그를 시작한다(기존 `FullScreenPointerArea` 회귀 보호).
- `RightClickDoesNotStartDrag` — 우클릭 후 마우스 이동이 `OnPreviewDragged`를 브로드캐스트하지 않는다.
- `CloseKeyStillWorksAfterRightClick` — 우클릭 이벤트 뒤 `NativeOnKeyDown(F)`가 `OnCloseRequested`를 발생시킨다.
- `ShouldRestoreWidgetFocus` 진리표 (뷰포트 포커스일 때만 true, 콘솔/타 위젯 포커스에서는 false).
- `ResolveInteractAction` 진리표.

수동 QA (`L_ItemInspectionTest` + 실제 고데기 오브젝트):

1. F로 인스펙터를 연다 → 우클릭 1회 → **F로 닫힌다**.
2. 같은 상황에서 Esc로도 닫힌다.
3. 우클릭을 누르고 있는 동안 커서가 사라지지 않는다.
4. 우클릭 후에도 좌드래그 회전 / 휠 줌이 정상이다.
5. 우클릭이 사진 카메라 모드를 켜지 않는다(인스펙터를 닫은 뒤에는 정상적으로 켜진다).
6. 닫은 뒤 이동/시야/커서/일시정지/소스 액터 숨김이 모두 복원된다.
7. 알트탭으로 창을 떠났다 돌아온 뒤에도 F로 닫힌다(워치독 확인).
8. 닫히는 전환 연출 중 F 연타가 이중 닫힘이나 재열림을 만들지 않는다.

## 7. 리스크

- **좌클릭 드래그 회귀**: 프리뷰 단계에서 좌클릭까지 `Handled`로 반환하면 드래그가 통째로 죽는다. 버튼 분기를 반드시 유지하고 테스트로 고정한다.
- **워치독의 포커스 탈취**: 뷰포트 포커스 조건이 빠지면 콘솔 입력을 먹는다. 조건을 순수 함수로 분리해 테스트로 고정한다.
- **BP 위젯 가시성**: `WBP_JMItemInspection` 루트가 히트 테스트 불가로 저장돼 있으면 계층 1이 동작하지 않는다. 구현 전 확인 항목.
- **플러그인 분기**: 업스트림 `Reuse_Plugin`과의 차이가 늘어난다. 커밋 분리로 관리한다.
