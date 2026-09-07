# 카메라·사진 시스템 통합 설계

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
제거해야 한다. 다음 중 하나로 담당자 2와 통합하며, 첫 번째 방식을 우선한다.

1. 플레이어 상호작용/UI 계층이 카메라 모드 상태를 받아 전체 라벨을 숨긴다.
2. 불가하면 인터페이스에 `SetCameraModeVisualSuppressed(bool)` 훅을 추가한다.

## 4. 정의 데이터 조회 계약

카메라와 HUD는 다음 기존 공개 API만 사용한다.

```text
GetEvidenceDefinition(ObjectID)          // ObjectName
GetEvidenceStateDefinition(StateID)      // ObservationText 및 상태 재검증
HasCapturedPhoto(PhotoID)                // ? / 체크 표시 및 중복 판정
RegisterCapturedPhoto(Record)            // 최종 등록
```

사진별 `CaptureSound`를 재생하려면 현재 private인 사진 정의 조회를 공개해야 한다.
담당자 1에게 다음 API 추가를 요청한다.

```cpp
UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
bool GetPhotoDefinition(FName PhotoID, FPhotoDefinition& OutDefinition) const;
```

갤러리 구현 시에는 별도로 아래 조회 API가 필요하다. 이번 카메라 촬영 통합에서 갤러리를
함께 이전하지 않는다면 후속 공통 계약으로 미뤄도 된다.

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
→ PendingPhotoIDs에 PhotoID 예약
→ 촬영 이미지 저장 요청
→ 저장 성공 콜백에서 FCapturedPhotoRecord 생성
→ RegisterCapturedPhoto
→ 성공 시 사운드, 저장 애니메이션, 성공 문구
→ PendingPhotoIDs 예약 해제
```

`PendingPhotoIDs`는 이미지 저장 콜백 전에 촬영 버튼이 반복 입력되어 같은 사진 파일을
여러 번 만드는 것을 막는다.

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

SceneCapture/Viewport 캡처 방식은 구현 단계에서 현재 화면과 동일한 FOV·후처리를
보존하는 쪽을 선택한다. HUD가 사진에 들어가지 않아야 한다면 `SceneCaptureComponent2D`
방식이 우선이고, 화면 그대로 저장해야 한다면 viewport screenshot 방식을 사용한다.

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
| `BalhwajeomCameraTargetInterface.h` | 성공 통지 제거, 필요 시 라벨 억제 훅 계약 추가 |
| `BalhwajeomPhotoCameraComponent.h/.cpp` | Subsystem 등록, pending 예약, 이미지 저장, 사운드/연출 연결, 자체 수집 제거 |
| `BalhwajeomEvidenceCameraHUD.h/.cpp` | 구체 EvidenceActor 의존 제거, Subsystem 기반 표시 |
| `BalhwajeomCameraCharacter.h/.cpp` | 자체 수집 목록 전달 API 제거 또는 Deprecated 처리 |
| `BalhwajeomInvestigationSubsystem.h/.cpp` | 담당자 1 협의 후 `GetPhotoDefinition` 공개 |
| `BalhwajeomEvidenceActor.h/.cpp` | 담당자 2가 새 인터페이스 스냅숏과 라벨 억제 계약 구현 |

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

### 플레이 검증

- 기존 진입/종료, WASD 이동, 회전, 줌 및 DOF가 유지된다.
- 거리 밖, 중앙 불일치, 화면 포함 비율 부족의 피드백이 유지된다.
- 카메라와 3인칭의 `?`/체크 표시가 일치한다.
- 상태 변경 후 HUD 문구와 `PhotoID`가 즉시 바뀐다.
- 저장된 PNG를 열 수 있고 `ImageRelativePath`로 다시 찾을 수 있다.

## 12. 구현 순서

1. 담당자 1과 `GetPhotoDefinition` 공개 API를 합의한다.
2. 담당자 2에게 새 `FBalhwajeomCameraTargetInfo`와 라벨 억제 계약을 전달한다.
3. 카메라 컴포넌트를 Subsystem 기반 판정과 등록으로 전환한다.
4. 실제 이미지 저장기와 pending/실패 보상 처리를 연결한다.
5. HUD를 구체 액터 의존 없이 Subsystem 조회 방식으로 전환한다.
6. Character와 블루프린트의 기존 자체 수집 API 사용처를 이전한다.
7. 자동 테스트와 플레이 검증 후 레거시 타입과 배열을 제거한다.
