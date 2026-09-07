# 카메라·사진 시스템 통합 설계

> 이 문서는 검토 과정의 상세 초안이다. 구현 기준은
> `Camera-Photo-System-Final-Design.md`를 사용한다.

> 상태: 구현 전 설계 확정안  
> 기준: `GetPhotoDefinition()` 공개 API 반영 완료  
> 비범위: 태블릿 갤러리, SaveGame, `ABalhwajeomEvidenceActor` 실제 구현

## 1. 목적과 범위

담당자 3의 기존 카메라 기능을 `PhotoID`와
`UBalhwajeomInvestigationSubsystem` 기반으로 전환한다.

이 문서의 구현 범위는 다음과 같다.

- 카메라 대상 인터페이스 계약 변경
- 현재 증거 상태 기반 초점 및 촬영 판정
- 실제 촬영 이미지 저장
- `FCapturedPhotoRecord` 등록
- `PhotoID` 기준 중복 촬영 표시
- 신규 사진 등록 시 사진별 사운드와 HUD 연출 실행
- 기존 카메라 자체 수집 상태 제거

`ABalhwajeomEvidenceActor`의 실제 구현은 담당자 2의 범위다. 담당자 3은
인터페이스 계약만 제공하고 카메라 코드에서 구체 액터 타입을 참조하지 않는다.

## 2. 현재 코드와 변경 원칙

현재 카메라는 다음 데이터를 자체 소유한다.

```text
FBalhwajeomEvidenceData
CollectedEvidence
CapturedFocusTargets
EvidenceData.bAlreadyCollected
```

통합 후 촬영 여부의 단일 원천은 다음 하나다.

```text
InvestigationSubsystem.HasCapturedPhoto(PhotoID)
```

따라서 같은 `PhotoID`는 다른 액터 인스턴스에서 찍어도 중복으로 간주한다. 반대로
같은 액터라도 상태가 변경되어 다른 `PhotoID`를 반환하면 새 사진으로 촬영할 수 있다.

### 2.1 책임 경계

| 책임 | 소유자 |
|---|---|
| 정적 증거·상태·사진 정의 | `UBalhwajeomInvestigationSubsystem` |
| 현재 증거 상태와 촬영 완료 기록 | `UBalhwajeomInvestigationSubsystem` |
| 현재 액터의 ID 스냅숏 제공 | `IBalhwajeomCameraTargetInterface` 구현체 |
| 거리·실루엣·중앙·프레이밍 판정 | `UBalhwajeomPhotoCameraComponent` |
| 화면 이미지 생성과 파일 저장 | `UBalhwajeomPhotoCameraComponent` 내부 저장 책임 |
| 카메라 HUD 표시 | `ABalhwajeomEvidenceCameraHUD` |
| 증거 액터의 상태 적용과 3인칭 라벨 | 담당자 2의 `ABalhwajeomEvidenceActor` |

카메라 컴포넌트는 판정과 촬영 작업을 조정하지만 조사 상태를 소유하지 않는다. HUD는
표시만 담당하고 촬영 등록이나 파일 저장을 시작하지 않는다.

## 3. 카메라 대상 인터페이스 계약

### 3.1 FBalhwajeomCameraTargetInfo

기존 `EvidenceData`, `InformationStages`, `bCanBeCaptured` 기반 정보를 다음 상태
스냅숏으로 교체한다.

```cpp
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FBalhwajeomCameraTargetInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target")
    FGuid EvidenceInstanceID;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target")
    FName ObjectID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target")
    FName StateID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target")
    FName PhotoID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target")
    bool bCanCapture = false;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target|Focus")
    float PreferredFocusDistance = 700.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target|Focus")
    float FocusDistanceTolerance = 300.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Camera Target|Focus")
    bool bScaleFocusDistanceWithZoom = true;
};
```

이 구조체는 액터의 현재 상태를 한 번에 전달하는 값 스냅숏이다. 카메라는 촬영 버튼을
누른 순간 `RequestCameraTargetInfo`를 다시 호출하여 초점 탐색 중 캐시된 상태를 그대로
신뢰하지 않는다.

### 3.2 인터페이스 함수

유지:

```text
RequestCameraTargetInfo
RequestCameraFocusLocation
RequestCameraFramingComponent
```

제거:

```text
NotifyCameraCaptureSucceeded
```

촬영 상태를 액터에 다시 기록하면 Subsystem과 이중 상태가 생기므로 성공 통지는 하지
않는다. 담당자 2의 액터와 3인칭 UI는 `OnPhotoCaptured`를 구독하거나
`HasCapturedPhoto(PhotoID)`를 조회한다.

카메라 모드에서 월드 라벨을 숨기는 기존 동작도 구체 `ABalhwajeomEvidenceActor` 캐스팅을
제거해야 한다. 이를 위해 인터페이스에 다음 통지 함수를 추가한다.

```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera Target")
void NotifyPhotoModeChanged(bool bIsPhotoModeActive);
```

카메라 모드 진입·종료 시 월드의 카메라 대상 인터페이스 구현체에 이 이벤트를 전달한다.
담당자 2의 EvidenceActor는 `true`일 때 3인칭 라벨을 숨기고 `false`일 때 현재 거리 상태로
라벨을 다시 평가한다. 이 함수는 촬영 상태를 변경하지 않는 시각적 통지만 담당한다.

## 4. 정의 데이터 조회 계약

카메라와 HUD는 다음 공개 API만 사용한다.

```text
GetEvidenceDefinition(ObjectID)          // ObjectName
GetEvidenceStateDefinition(StateID)      // ObservationText 및 상태 재검증
GetPhotoDefinition(PhotoID)              // PhotoName 및 CaptureSound
HasCapturedPhoto(PhotoID)                // ? / 체크 표시 및 중복 판정
RegisterCapturedPhoto(Record)            // 최종 등록
```

`GetPhotoDefinition()`은 담당자 1의 공통 API에 반영되어 있으며, 기존 private
`FindPhotoDefinition()`의 결과를 값으로 복사한다.

```cpp
UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
bool GetPhotoDefinition(FName PhotoID, FPhotoDefinition& OutDefinition) const;
```

갤러리 구현 시에는 별도로 아래 조회 API가 필요하다. 이번 카메라 촬영 통합에서는 추가하지
않고 후속 공통 계약으로 미룬다.

```cpp
UFUNCTION(BlueprintCallable, Category = "Investigation|Photos")
void GetCapturedPhotoRecords(TArray<FCapturedPhotoRecord>& OutRecords) const;
```

카메라가 Settings 또는 DataTable을 직접 읽는 방식은 사용하지 않는다.

## 5. 대상 탐색과 초점 판정

기존 실루엣, 중앙점, 화면 포함 비율 및 DOF 코드는 유지한다. 거리 파라미터의 공급원만
인터페이스의 현재 상태 스냅숏으로 바꾼다.

```text
RequestCameraTargetInfo
→ ID 유효성 확인
→ GetEvidenceStateDefinition(StateID)
→ State.ObjectID == TargetInfo.ObjectID 확인
→ 인터페이스 값과 상태 정의의 촬영 필드 일치 확인
→ 거리 밴드 판정
→ 실루엣 가시성 판정
→ 화면 중앙 및 70% 포함 판정
```

인터페이스가 반환한 상태 필드와 DataTable 값이 다르면 해당 프레임의 대상을 유효하지
않은 것으로 처리하고 로그를 남긴다. 이렇게 하면 담당자 2의 상태 적용 누락을 조기에
찾을 수 있다.

`bCanCapture == false`인 대상도 관찰용 초점 대상으로 보여 줄 수는 있지만 `PhotoID`가
없으므로 `?` 또는 체크 표시는 그리지 않고 촬영 등록은 거부한다.

### 5.1 유효한 대상 스냅숏 조건

아래 조건을 모두 만족해야 촬영 후보가 된다.

```text
EvidenceInstanceID.IsValid()
ObjectID != None
StateID != None
GetEvidenceStateDefinition(StateID) 성공
State.ObjectID == TargetInfo.ObjectID
State.bCanCapture == TargetInfo.bCanCapture
State.PhotoID == TargetInfo.PhotoID
State의 세 거리 설정 == TargetInfo의 세 거리 설정
```

부동소수점 거리 값 비교에는 `FMath::IsNearlyEqual`을 사용한다. 불일치는 데이터 오류로
간주하되 매 프레임 로그가 쌓이지 않도록 대상과 상태 조합별 최초 한 번만 경고한다.

## 6. 촬영 성공 트랜잭션

셔터 플래시는 촬영 입력 자체의 피드백이므로 기존처럼 항상 실행할 수 있다. 증거 획득
연출과 전용 사운드는 아래 절차가 모두 성공한 뒤에만 실행한다.

```text
TakePhoto
→ 현재 초점 즉시 갱신
→ 중심/화면 포함 비율/거리 조건 확인
→ RequestCameraTargetInfo 재호출
→ 필수 ID와 bCanCapture 확인
→ GetEvidenceStateDefinition으로 ObjectID, StateID, PhotoID 재검증
→ HasCapturedPhoto(PhotoID) 확인
→ PendingCapture에 촬영 스냅숏 예약
→ 촬영 이미지 저장 요청
→ 저장 성공 콜백에서 FCapturedPhotoRecord 생성
→ RegisterCapturedPhoto
→ 성공 시 사운드, 저장 애니메이션, 성공 문구
→ PendingCapture 예약 해제
```

viewport screenshot은 프로세스 전역 요청을 사용하므로 동시에 하나의 촬영만 허용한다.
`PendingCapture`가 존재하는 동안 추가 촬영 입력은 저장 요청을 만들지 않고 짧은 대기
피드백만 표시한다.

```cpp
struct FPendingPhotoCapture
{
    FGuid RequestID;
    FBalhwajeomCameraTargetInfo TargetSnapshot;
    FDateTime RequestedTime;
    FString RelativePath;
    FString AbsolutePath;
};

TOptional<FPendingPhotoCapture> PendingCapture;
```

`RequestID`는 늦게 도착한 콜백이 이미 교체되거나 취소된 요청을 완료하지 못하게 한다.
이번 설계에서는 한 번에 한 요청만 허용하지만 콜백 식별자는 명시적으로 유지한다.

### 6.1 FCapturedPhotoRecord

등록 시 촬영 버튼을 누른 순간의 스냅숏을 사용한다.

```text
PhotoID           = TargetInfo.PhotoID
ObjectID          = TargetInfo.ObjectID
EvidenceInstanceID= TargetInfo.EvidenceInstanceID
CapturedStateID   = TargetInfo.StateID
ImageRelativePath = 이미지 저장기가 반환한 상대 경로
CapturedTime      = UtcNow
bViewedInTablet   = false
```

비동기 저장 중 증거 상태가 바뀌면 `RegisterCapturedPhoto`가 오래된 상태의 등록을
거부한다. 이 경우 저장된 고아 파일을 삭제하고 성공 연출과 전용 사운드는 실행하지 않는다.

### 6.2 이미지 저장 경로

저장 루트는 패키징 환경에서도 쓰기 가능한 `ProjectSavedDir`을 사용한다.

```text
절대 경로: <ProjectSavedDir>/Investigation/Photos/<SessionID>/<PhotoID>.png
레코드 값: Investigation/Photos/<SessionID>/<PhotoID>.png
```

`PhotoID`는 파일명에 사용하기 전에 파일시스템에 안전한 문자열로 정규화한다. 현재
SaveGame 범위가 아니므로 세션 폴더를 분리해 이전 실행의 파일과 충돌하지 않게 한다.

이미지 저장은 카메라 컴포넌트 내부의 별도 작은 책임으로 분리한다.

```text
RequestSavePhoto(PhotoID, Completion)
Completion(bool bSucceeded, FString RelativePath)
```

### 6.3 이미지 캡처 방식

1차 구현은 **viewport one-shot screenshot** 방식으로 확정한다.

- 현재 플레이어가 보는 카메라의 FOV와 후처리를 그대로 얻는다.
- 요청 시 UI 포함 옵션을 끄므로 카메라 HUD와 플래시는 이미지에서 제외한다.
- 캡처 완료 델리게이트는 요청 직전에 등록하고, 최초 유효 콜백에서 즉시 해제한다.
- 캡처된 픽셀을 PNG로 저장한 뒤 파일 존재 여부와 파일 크기를 확인한다.
- 요청 등록 실패, 빈 픽셀, PNG 저장 실패는 모두 이미지 저장 실패로 처리한다.

플랫폼 빌드에서 UI 제외나 픽셀 콜백이 동작하지 않는 것이 확인될 때만
`SceneCaptureComponent2D + RenderTarget` 방식으로 교체한다. 저장기 경계를 분리하므로
이 교체는 촬영 판정과 Subsystem 등록 흐름을 바꾸지 않는다.

### 6.4 촬영 상태 머신

```text
Idle
 └─ 유효한 촬영 입력 → WaitingForPixels
      ├─ 픽셀/파일 저장 실패 → Failed → Idle
      └─ 파일 저장 성공 → Registering
           ├─ Subsystem 등록 실패 → 파일 삭제 → Failed → Idle
           └─ Subsystem 등록 성공 → Completed → Idle
```

`ExitCameraMode()`가 호출되어도 이미 셔터가 눌린 요청은 완료시킨다. 컴포넌트 또는 World가
파괴되면 델리게이트를 해제하고 pending 상태를 폐기한다. 이때 아직 쓰지 않은 파일 경로는
삭제하지 않으며, 실제 파일 쓰기가 완료된 뒤 등록 실패한 경우에만 정확한 파일을 삭제한다.

### 6.5 실패 결과와 사용자 피드백

카메라 내부 판정은 단순 `bool` 대신 로컬 결과 enum으로 분류한다.

```text
NoFocusedTarget
NotFramedEnough
NotCentered
InvalidTargetSnapshot
CaptureDisabled
AlreadyCaptured
CapturePending
ImageSaveFailed
RegistrationRejected
Succeeded
```

이 enum은 공통 조사 API에 추가하지 않는다. 화면 문구 선택, 로그, 자동 테스트를 위한
카메라 전용 결과다. `RegisterCapturedPhoto()`의 상세 실패 원인은 공통 API가 노출하지
않으므로 카메라는 사전 검사 결과와 최종 `false`만 구분한다.

## 7. 등록 이후 연출

`RegisterCapturedPhoto`가 `true`를 반환한 경우에만 다음을 실행한다.

```text
GetPhotoDefinition(PhotoID)
→ CaptureSound.LoadSynchronous()
→ UGameplayStatics::PlaySound2D

GetEvidenceDefinition(ObjectID)
→ ObjectName으로 TriggerEvidenceSavedAnimation

성공 피드백 표시
```

사운드 에셋이 비어 있거나 로드에 실패해도 사진 등록 성공 자체는 되돌리지 않는다.
경고만 남기고 나머지 연출을 계속한다.

셔터 입력음과 사진별 획득음은 구분한다.

```text
셔터 입력음       = 유효한 카메라 모드에서 촬영 버튼을 누를 때 재생
사진별 CaptureSound = 신규 사진이 Subsystem에 등록된 뒤에만 재생
```

두 사운드가 같은 에셋이어도 실행 조건은 합치지 않는다.

## 8. HUD 설계

`ABalhwajeomEvidenceCameraHUD`는 더 이상 `ABalhwajeomEvidenceActor` 또는
`UInspectionComponent`를 캐스팅하거나 읽지 않는다.

```text
DisplayedFocusTargetInfo.PhotoID
→ 유효하고 bCanCapture이면 HasCapturedPhoto(PhotoID)
→ false: ?
→ true: 체크

DisplayedFocusTargetInfo.StateID
→ GetEvidenceStateDefinition
→ ObservationText 표시

DisplayedFocusTargetInfo.ObjectID
→ GetEvidenceDefinition
→ ObjectName 표시
```

상태가 바뀐 직후에는 다음 HUD 스냅숏 갱신에서 새 `StateID`와 `PhotoID`가 반영된다.

HUD의 `DrawHUD()`가 같은 프레임에 정의를 여러 번 조회하지 않도록 한 번의 그리기에서
Subsystem 포인터와 정의 결과를 지역 변수로 재사용한다. 조회 실패 시 대상 이름과 설명은
숨기며 `?` 또는 체크도 표시하지 않는다.

### 8.1 촬영 이벤트와 3인칭 표시 동기화

- 카메라 HUD는 매 그리기 시 `HasCapturedPhoto(PhotoID)`를 조회하므로 별도 로컬 상태가 없다.
- 담당자 2의 액터는 `OnPhotoCaptured`를 구독해 현재 상태의 `PhotoID`와 일치할 때 라벨을
  다시 그린다.
- 상태 변경 이벤트를 받으면 담당자 2가 `CurrentStateID`를 갱신하고, 카메라는 다음 focus
  refresh에서 새 스냅숏을 받는다.
- 이벤트 구독 객체는 종료 시 반드시 delegate handle을 해제한다.

## 9. 제거 및 단계적 이전

카메라 통합 완료 후 제거한다.

```text
FBalhwajeomEvidenceData
CollectedEvidence
CapturedFocusTargets
AddEvidence
HasEvidence
GetCollectedEvidence
MarkAsCollected 호출
NotifyCameraCaptureSucceeded
EvidenceData.bAlreadyCollected 기반 HUD 판정
```

블루프린트가 `GetCollectedEvidence`를 참조하고 있다면 즉시 함수를 삭제하지 말고
Deprecated 메타를 붙인 뒤 갤러리의 Subsystem 조회 API 이전과 함께 제거한다. 단, 이전
기간에도 이 배열을 촬영 여부의 판정에 사용해서는 안 된다.

## 10. 파일별 변경 계획

| 파일 | 변경 내용 |
|---|---|
| `BalhwajeomEvidenceTypes.h` | 대상 정보를 ID/상태 스냅숏으로 변경, 기존 증거 데이터 제거 준비 |
| `BalhwajeomCameraTargetInterface.h` | 성공 통지 제거, `NotifyPhotoModeChanged` 계약 추가 |
| `BalhwajeomPhotoCameraComponent.h/.cpp` | Subsystem 등록, pending 예약, 이미지 저장, 사운드/연출 연결, 자체 수집 제거 |
| `BalhwajeomEvidenceCameraHUD.h/.cpp` | 구체 EvidenceActor 의존 제거, Subsystem 기반 표시 |
| `BalhwajeomCameraCharacter.h/.cpp` | 자체 수집 목록 전달 API 제거 또는 Deprecated 처리 |
| `BalhwajeomInvestigationSubsystem.h/.cpp` | `GetPhotoDefinition` 반영 완료, 추가 변경 없음 |
| `BalhwajeomEvidenceActor.h/.cpp` | 담당자 2가 새 인터페이스 스냅숏과 라벨 억제 계약 구현 |

### 10.1 컴포넌트 내부 함수 분해안

```cpp
UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;

bool ResolveAndValidateTargetSnapshot(
    AActor* Target,
    FBalhwajeomCameraTargetInfo& OutInfo,
    FEvidenceStateDefinition& OutState) const;

EPhotoCaptureResult EvaluateCapture(
    AActor* Target,
    FBalhwajeomCameraTargetInfo& OutInfo) const;

bool BeginImageCapture(const FBalhwajeomCameraTargetInfo& TargetInfo);
void HandleScreenshotCaptured(/* pixel payload */);
void CompleteCapturedPhoto(const FString& RelativePath);
void FailPendingCapture(EPhotoCaptureResult FailureReason);
void PlayRegisteredPhotoFeedback(const FCapturedPhotoRecord& Record);
void ClearScreenshotDelegate();
```

`TryCaptureActiveFocusTarget()`은 위 함수들을 조정하는 얇은 진입점으로 유지한다. 파일 저장,
Subsystem 등록, HUD 문구가 한 함수에 뒤섞이지 않게 한다.

## 11. 테스트 기준

### 자동 테스트

- 캡처 불가 상태는 등록 요청까지 가지 않는다.
- 잘못된 `ObjectID`, `StateID`, `PhotoID` 조합은 거부된다.
- 같은 `PhotoID`의 연속 입력은 pending 단계에서 한 번만 저장 요청한다.
- 같은 `PhotoID`는 다른 액터 인스턴스에서도 두 번째 등록이 실패한다.
- 같은 액터가 새 상태에서 다른 `PhotoID`를 반환하면 등록할 수 있다.
- 이미지 저장 실패 시 Subsystem 등록, 사운드, 저장 애니메이션이 실행되지 않는다.
- 이미지 저장 후 Subsystem 등록 실패 시 고아 파일을 정리한다.
- `RegisterCapturedPhoto == true`인 경우에만 전용 사운드가 한 번 재생된다.
- pending 중 두 번째 입력은 별도 저장 요청을 만들지 않는다.
- 늦게 도착한 다른 RequestID의 콜백은 무시한다.
- 카메라 모드 종료 후 도착한 정상 콜백은 한 번만 등록된다.
- 컴포넌트 파괴 시 screenshot delegate가 남지 않는다.

### 플레이 검증

- 기존 진입/종료, WASD 이동, 회전, 줌 및 DOF가 유지된다.
- 거리 밖, 중앙 불일치, 화면 포함 비율 부족의 피드백이 유지된다.
- 카메라와 3인칭의 `?`/체크 표시가 일치한다.
- 상태 변경 후 HUD 문구와 `PhotoID`가 즉시 바뀐다.
- 저장된 PNG를 열 수 있고 `ImageRelativePath`로 다시 찾을 수 있다.

## 12. 구현 순서

1. 완료된 `GetPhotoDefinition` API를 기준선으로 고정한다.
2. 담당자 2에게 새 `FBalhwajeomCameraTargetInfo`와 라벨 억제 계약을 전달한다.
3. 타입과 인터페이스를 먼저 변경하고 담당자 2의 컴파일 대응 지점을 확인한다.
4. 카메라 컴포넌트를 Subsystem 기반 판정으로 전환한다.
5. viewport 이미지 저장기와 pending 상태 머신을 연결한다.
6. `FCapturedPhotoRecord` 등록과 사운드·연출을 연결한다.
7. HUD를 구체 액터 의존 없이 Subsystem 조회 방식으로 전환한다.
8. Character와 블루프린트의 기존 자체 수집 API 사용처를 이전한다.
9. 자동 테스트와 플레이 검증 후 레거시 타입과 배열을 제거한다.

## 13. 담당자 2에게 전달할 최소 계약

담당자 2는 `ABalhwajeomEvidenceActor`에서 다음만 보장한다.

```text
BeginPlay에서 RegisterEvidenceActor 성공
RequestCameraTargetInfo가 현재 EvidenceInstanceID/ObjectID/CurrentStateID를 반환
현재 StateDefinition의 PhotoID와 촬영 거리 설정을 같은 스냅숏에 복사
RequestCameraFocusLocation이 CameraFocusPoint 위치 반환
RequestCameraFramingComponent가 실제 증거 메시 반환
NotifyPhotoModeChanged에 따라 3인칭 라벨 숨김/복원
OnPhotoCaptured 수신 시 3인칭 라벨 재평가
```

담당자 3은 액터의 `CurrentStateID`를 직접 변경하거나 `MarkAsCollected()`를 호출하지 않는다.

## 14. 완료 정의

다음 조건을 모두 만족해야 카메라·사진 통합 완료로 판단한다.

- 기존 카메라 조작과 초점 연출이 회귀하지 않는다.
- 카메라 코드에서 `ABalhwajeomEvidenceActor` include와 cast가 제거된다.
- 촬영 여부를 결정하는 자체 배열과 액터 bool이 제거된다.
- 유효한 신규 사진 한 장당 실제 PNG 한 개와 Subsystem record 한 개가 생긴다.
- 실패한 촬영은 record, 획득 사운드, 저장 애니메이션을 만들지 않는다.
- 등록 실패로 남는 고아 PNG가 없다.
- 카메라와 3인칭의 `?`/체크가 같은 `PhotoID` 기준으로 일치한다.
- 상태 변경 후 새 `PhotoID`를 촬영할 수 있다.
- 자동 테스트와 에디터 플레이 검증을 통과한다.
