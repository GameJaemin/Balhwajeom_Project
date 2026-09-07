# 카메라·사진 시스템 최종 설계

> 이 문서는 위험 점검 결과를 반영한 구현 기준 문서다. 기능 구현은 아래 단계와 통과
> 조건을 순서대로 따른다.

현재 구현 상태:

- 호환 가능한 Investigation target 필드 추가 완료
- DataTable 상태 기준 target 정규화 완료
- Subsystem 중복 조회 및 촬영 기록 등록 경로 완료
- 단일 pending viewport screenshot 및 비동기 PNG 저장 완료
- Subsystem 기반 카메라 HUD 상태 조회 완료
- 담당자 2의 EvidenceActor 새 ID/상태 스냅숏 연결 대기
- 실제 맵·패키지 이미지 캡처 검증 대기

## 1. 설계 목표

기존 카메라의 이동, 회전, 줌, 초점, 실루엣, 중앙 판정과 프레이밍 판정은 유지하면서
사진 수집 상태를 `UBalhwajeomInvestigationSubsystem`으로 일원화한다.

이번 구현의 완료 범위는 다음 세 가지다.

```text
현재 증거 상태로 촬영 가능 여부 판정
→ 실제 이미지 파일 저장
→ FCapturedPhotoRecord를 Subsystem에 등록
```

2D HUD와 3D 카메라 모델 중 무엇을 사용할지, 조준선과 저장 애니메이션을 어떻게 표현할지
등의 연출은 후속 작업으로 둔다.

## 2. 데이터 관계

```text
EvidenceActor
 ├─ EvidenceInstanceID
 ├─ ObjectID
 └─ CurrentStateID
       ↓
EvidenceStateDefinition
 ├─ bCanCapture
 ├─ PhotoID ───────────────→ PhotoDefinition
 ├─ PreferredFocusDistance       ├─ PhotoName
 ├─ FocusDistanceTolerance       ├─ DescriptionSource
 └─ bScaleFocusDistanceWithZoom  ├─ CustomDescription
                                  ├─ PhotoSentenceID ─→ SentenceDefinition
                                  ├─ PhotoTags
                                  └─ CaptureSound

촬영 성공
  ↓
CapturedPhotoRecord
 ├─ PhotoID
 ├─ ObjectID
 ├─ EvidenceInstanceID
 ├─ CapturedStateID
 ├─ ImageRelativePath
 ├─ CapturedTime
 └─ bViewedInTablet
```

### 2.1 정적 데이터와 런타임 데이터

| 데이터 | 성격 | 소유자 |
|---|---|---|
| `PhotoDefinition` | 사진의 의미와 표시·분석 메타데이터 | DataTable / InvestigationSubsystem |
| `SentenceDefinition` | 사진과 키워드를 사용하는 문제 정의 | DataTable / InvestigationSubsystem |
| `CapturedPhotoRecord` | 플레이 중 실제로 생성된 촬영 결과 | InvestigationSubsystem |

카메라는 `PhotoDefinition`이나 `SentenceDefinition`을 수정하지 않는다. 카메라가 생성하는
데이터는 `CapturedPhotoRecord`뿐이다.

## 3. 제공된 데이터 구조의 해석

### 3.1 PhotoDefinition

| 필드 | 카메라에서의 사용 |
|---|---|
| `PhotoID` | 상태와 사진 정의 및 촬영 기록을 연결하는 핵심 ID |
| `PhotoName` | 후속 HUD·갤러리 표시용; 핵심 촬영 판정에는 사용하지 않음 |
| `DescriptionSource` | 후속 갤러리 설명 결정용 |
| `CustomDescription` | `DescriptionSource == Custom`일 때 후속 갤러리가 사용 |
| `PhotoSentenceID` | 사진 분석 시스템 연결용; 카메라는 해석하지 않음 |
| `PhotoTags` | 후속 분류·필터용; 카메라는 해석하지 않음 |
| `CaptureSound` | 선택적 촬영 성공 연출용; 핵심 등록 흐름과 분리 |

`CaptureSound`가 비어 있어도 촬영과 등록은 정상 성공해야 한다. 모든 사진이 같은 사운드를
사용한다면 카메라 공통 사운드를 사용하고 이 필드는 비워 둘 수 있다. 사진별 사운드가
필요해질 때는 등록 성공 후 `GetPhotoDefinition(PhotoID)`으로 조회한다.

### 3.2 SentenceDefinition

`SentenceDefinition`은 촬영 이후의 사진 분석 및 진술 완성 시스템이 사용한다. 카메라는
`PhotoSentenceID`의 존재 여부나 문장 정답을 검사하지 않는다. 해당 참조의 유효성은
Subsystem의 DataTable 검증 책임이다.

### 3.3 CapturedPhotoRecord

카메라는 촬영 버튼을 누른 순간의 상태 스냅숏과 이미지 저장 결과로 아래 값을 만든다.

```text
PhotoID            = 촬영 당시 EvidenceStateDefinition.PhotoID
ObjectID           = 대상 EvidenceActor.ObjectID
EvidenceInstanceID = 대상 EvidenceActor.EvidenceInstanceID
CapturedStateID    = 대상 EvidenceActor.CurrentStateID
ImageRelativePath  = 저장 성공한 이미지의 ProjectSavedDir 기준 상대 경로
CapturedTime       = 촬영 요청 시각의 UTC
bViewedInTablet    = false
```

제공된 표의 `AcquiredWordIDs`는 최종 C++ `FCapturedPhotoRecord`에 포함하지 않는다. 확정된
규칙상 사진 촬영에서 키워드를 획득하지 않으며 현재 런타임 타입에도 이 필드가 없다.

## 4. 시스템별 책임

### 4.1 InvestigationSubsystem

- 증거·상태·사진 정의 조회
- 증거 인스턴스의 현재 상태 관리
- `PhotoID` 기준 중복 촬영 차단
- 촬영 기록의 인스턴스·오브젝트·상태·사진 관계 검증
- 유효한 `FCapturedPhotoRecord` 저장
- `OnPhotoCaptured` 발생

### 4.2 CameraTargetInterface 구현체

- 현재 증거 인스턴스와 상태 ID 제공
- 카메라 초점 위치 제공
- 화면 포함 비율 계산에 사용할 메시 컴포넌트 제공
- 현재 상태의 촬영 설정 스냅숏 제공

### 4.3 PhotoCameraComponent

- 대상 탐색
- 초점 거리, 가시성, 중앙점 및 화면 포함 비율 판정
- 촬영 순간 대상 상태 재조회
- 이미지 캡처와 파일 저장
- `FCapturedPhotoRecord` 생성과 등록 요청
- 촬영 진행 중 중복 입력 차단
- 성공·실패 결과를 후속 연출 계층에 전달

카메라 컴포넌트는 액터에서 받은 원시 스냅숏을 바로 사용하지 않고 내부의
`FResolvedPhotoTarget`으로 정규화한 뒤 이후 단계에 전달한다.

```cpp
struct FResolvedPhotoTarget
{
    FGuid EvidenceInstanceID;
    FName ObjectID = NAME_None;
    FName StateID = NAME_None;
    FName PhotoID = NAME_None;
    bool bCanCapture = false;
    float PreferredFocusDistance = 700.0f;
    float FocusDistanceTolerance = 300.0f;
    bool bScaleFocusDistanceWithZoom = true;
};
```

ID 세 개는 인터페이스 스냅숏에서 가져오고, `PhotoID`와 촬영 설정은
`GetEvidenceStateDefinition(StateID)` 결과를 최종 권위로 사용한다. 인터페이스에 복사된
촬영 값이 다르면 경고를 남기지만 DataTable 값으로 정규화한다. 이렇게 하면 중복 필드의
불일치가 촬영 시스템 전체를 멈추게 하지 않는다.

### 4.4 HUD·3D 카메라·태블릿

후속 연출 및 UI 책임이다. 촬영 완료 상태를 별도로 저장하지 않고 반드시
`HasCapturedPhoto(PhotoID)` 또는 `OnPhotoCaptured`를 사용한다.

## 5. CameraTarget 인터페이스 계약

`FBalhwajeomCameraTargetInfo`는 현재 상태의 값 스냅숏이다.

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

유지할 인터페이스 함수:

```text
RequestCameraTargetInfo
RequestCameraFocusLocation
RequestCameraFramingComponent
```

제거할 인터페이스 함수:

```text
NotifyCameraCaptureSucceeded
```

촬영 성공을 액터에 기록하면 Subsystem과 이중 상태가 생긴다. 액터와 3인칭 UI는
`OnPhotoCaptured`를 구독하거나 필요할 때 `HasCapturedPhoto(PhotoID)`를 조회한다.

카메라 모드에서 월드 라벨을 숨기는 방법은 2D/3D 연출 결정과 함께 후속 설계한다. 이
시각적 요구 때문에 핵심 촬영 인터페이스를 지금 확장하지 않는다.

## 6. 사용하는 공통 API

```cpp
bool GetEvidenceDefinition(
    FName ObjectID,
    FEvidenceDefinition& OutDefinition) const;

bool GetEvidenceStateDefinition(
    FName StateID,
    FEvidenceStateDefinition& OutState) const;

bool GetPhotoDefinition(
    FName PhotoID,
    FPhotoDefinition& OutDefinition) const;

bool HasCapturedPhoto(FName PhotoID) const;

bool RegisterCapturedPhoto(
    const FCapturedPhotoRecord& Record);
```

카메라는 `InvestigationSettings`나 DataTable을 직접 읽지 않는다. `GetPhotoDefinition()`은
공통 API에 구현되어 있으며 사진별 성공 연출이 필요할 때만 사용한다.

사진 목록 조회 API는 이번 범위에 필요하지 않다. 태블릿 갤러리 구현 시 Subsystem에
별도로 추가한다.

## 7. 촬영 후보 판정

### 7.1 초점 후보

기존 카메라 기능을 유지한다.

```text
CameraTargetInterface 구현 여부
→ 초점 위치가 카메라 전방 및 탐색 거리 안인지 확인
→ 현재 상태의 초점 거리 밴드 확인
→ 실루엣 가시성 확인
→ 화면 중심에 가장 가까운 대상 선택
→ DOF 적용
```

`bCanCapture == false`인 대상도 관찰용 초점 후보가 될 수 있다. 다만 촬영 성공 후보는
아니다.

### 7.2 촬영 버튼 시 최종 검사

초점 탐색 중 저장한 정보가 오래되었을 수 있으므로 촬영 버튼을 누른 순간
`RequestCameraTargetInfo()`를 다시 호출한다.

```text
카메라 모드이며 전환 중이 아님
PendingCapture가 없음
ActiveFocusTarget가 유효함
화면 포함 비율 >= MinimumCaptureCoverageRatio
화면 중앙 ray가 대상에 닿음
EvidenceInstanceID가 유효함
ObjectID, StateID가 None이 아님
GetEvidenceStateDefinition(StateID) 성공
State.ObjectID == TargetInfo.ObjectID
State.bCanCapture == true
State.PhotoID가 None이 아님
State.PhotoID와 촬영 거리 설정을 최종 값으로 사용
TargetInfo의 복사 값이 다르면 경고 후 State 값으로 정규화
HasCapturedPhoto(PhotoID) == false
```

이 검사를 통과한 스냅숏만 이미지 캡처 단계로 넘긴다. 부동소수점 거리 설정 비교에는
`FMath::IsNearlyEqual()`을 사용한다.

## 8. 이미지 저장

### 8.1 1차 구현 방식

현재 플레이어 카메라의 FOV와 후처리를 그대로 저장하기 위해 viewport one-shot
screenshot 방식을 사용한다.

- 요청 전에 `FScreenshotRequest::IsScreenshotRequested()`를 확인한다. 다른 시스템의
  screenshot 요청이 있으면 덮어쓰지 않고 이번 촬영을 재시도 가능한 실패로 처리한다.
- `FScreenshotRequest::OnScreenshotCaptured()`와
  `OnScreenshotRequestProcessed()`를 함께 감시한다. 픽셀 콜백 없이 processed만 발생하면
  실패로 처리하여 pending 상태가 영구히 남지 않게 한다.
- `OnScreenshotCaptured`는 프로세스 전역 multicast delegate이므로 자신의
  `FDelegateHandle`만 제거하며 다른 구독자를 해제하지 않는다.
- captured delegate가 연결된 경우 UE가 자동 파일 저장 대신 픽셀을 delegate로 전달하므로
  카메라 저장기가 `FImageUtils::SaveImageByExtension()`으로 직접 PNG를 저장한다.
- UI 포함 옵션은 끈다. UE 5.7에서 이 옵션은 Slate UI 제외를 명시적으로 보장하지만 Canvas
  HUD 제외까지 계약하지는 않으므로 실제 플레이 검증을 통과해야 한다.
- 카메라 HUD, 조준선, 플래시와 향후 2D 연출은 최종 저장 이미지에 포함하지 않는 것을
  제품 요구사항으로 둔다. Canvas HUD가 포함되면 캡처 프레임 동안 HUD를 억제하거나
  `SceneCaptureComponent2D` 방식으로 전환한다.
- 향후 3D 카메라 모델이 추가되면 해당 모델도 캡처에서 제외한다.
- screenshot 완료 델리게이트는 요청 직전에 등록하고 완료·실패·파괴 시 해제한다.
- 플랫폼에서 viewport 캡처가 요구를 만족하지 못할 때만 저장기 구현을
  `SceneCaptureComponent2D + RenderTarget`으로 교체한다.

### 8.2 저장 경로

```text
절대 경로
<ProjectSavedDir>/Investigation/Photos/<SessionID>/<SafePhotoID>_<RequestID>.png

FCapturedPhotoRecord.ImageRelativePath
Investigation/Photos/<SessionID>/<SafePhotoID>_<RequestID>.png
```

- `PhotoID`는 파일명으로 사용하기 전에 안전한 문자열로 정규화한다.
- 정규화 결과가 같은 두 ID의 충돌과 이전 실패 파일 덮어쓰기를 막기 위해 `RequestID`를
  파일명에 포함한다.
- 현재 SaveGame 범위가 아니므로 실행마다 새로운 `SessionID`를 사용한다.
- 디렉터리 생성, PNG 쓰기, 파일 존재 및 파일 크기 확인까지 성공해야 저장 성공이다.
- 비정상 종료로 등록 전에 남은 파일은 다음 실행 시 이전 세션 폴더 정리 정책으로 제거한다.

## 9. 촬영 트랜잭션

viewport screenshot 요청은 동시에 하나만 처리한다.

```cpp
struct FPendingPhotoCapture
{
    FGuid RequestID;
    FResolvedPhotoTarget TargetSnapshot;
    FDateTime RequestedTime;
    FString RelativePath;
    FString AbsolutePath;
};

TOptional<FPendingPhotoCapture> PendingCapture;
```

```text
Idle
→ 최종 촬영 검사 성공
→ PendingCapture 생성
→ screenshot 요청
→ 픽셀 수신
→ PNG 저장
→ FCapturedPhotoRecord 생성
→ RegisterCapturedPhoto
```

### 9.1 성공

`RegisterCapturedPhoto()`가 `true`를 반환한 경우에만 촬영 성공이다.

```text
OnPhotoCaptured 발생
→ PendingCapture 해제
→ 성공 결과를 연출 계층에 전달
→ 선택적으로 CaptureSound 처리
```

### 9.2 실패와 보상

| 실패 지점 | 처리 |
|---|---|
| 촬영 조건 실패 | 이미지 요청 없이 종료 |
| 이미 촬영한 `PhotoID` | 이미지 요청 없이 종료 |
| pending 중 추가 입력 | 새 요청을 만들지 않음 |
| screenshot 실패 | pending 해제, 등록하지 않음 |
| PNG 저장 실패 | pending 해제, 등록하지 않음 |
| 저장 후 Subsystem 등록 실패 | 이번 요청으로 만든 정확한 PNG 삭제 후 pending 해제 |
| 컴포넌트·World 파괴 | delegate 해제, 등록 중단 |
| 다른 전역 screenshot 요청 진행 중 | 기존 요청을 보존하고 이번 촬영은 시작하지 않음 |

비동기 저장 중 대상 액터가 사라져도 스냅숏은 값으로 보관한다. 다만 그 사이 Subsystem의
현재 상태가 바뀌면 `RegisterCapturedPhoto()`가 오래된 `CapturedStateID`를 거부하고 파일을
정리한다.

카메라 모드를 나간 뒤 정상 screenshot 콜백이 도착하면 이미 누른 촬영은 끝까지 처리한다.

PNG 압축과 파일 쓰기는 고해상도에서 게임 스레드를 멈출 수 있다. 픽셀 배열은 콜백에서
소유 복사한 뒤 worker thread에서 압축·저장하고, Subsystem 접근과 연출 통지는 game
thread로 돌아와 실행한다. worker에서는 UObject와 `PendingCapture`를 직접 접근하지 않고
RequestID와 값 데이터만 사용한다.

## 10. 구현 구조

`TryCaptureActiveFocusTarget()`은 흐름만 조정하고 판정, 저장, 등록을 분리한다.

```cpp
UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;

bool ResolveCurrentTargetSnapshot(
    AActor* Target,
    FResolvedPhotoTarget& OutTarget) const;

EPhotoCaptureResult EvaluateCapture(
    AActor* Target,
    FResolvedPhotoTarget& OutTarget) const;

bool BeginImageCapture(
    const FResolvedPhotoTarget& Target);

void HandleScreenshotCaptured(/* pixel data */);
void RegisterSavedPhoto();
void FailPendingCapture(EPhotoCaptureResult Reason);
void ClearScreenshotDelegate();
```

카메라 전용 결과 enum:

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

이 enum은 공통 API에 추가하지 않는다. 후속 HUD 피드백과 자동 테스트를 위한 카메라 내부
결과다.

### 10.1 이미지 캡처 백엔드 경계

전역 `FScreenshotRequest` 의존성을 카메라 판정 코드에 직접 흩뿌리지 않는다.

```cpp
struct FPhotoImageCaptureRequest
{
    FGuid RequestID;
    FString AbsolutePath;
};

struct FPhotoImageCaptureResult
{
    FGuid RequestID;
    bool bSucceeded = false;
    FString AbsolutePath;
};

class FPhotoImageCaptureBackend
{
public:
    bool BeginCapture(const FPhotoImageCaptureRequest& Request);
    void CancelOwnerRequests();
};
```

백엔드는 screenshot delegate 등록·해제, 픽셀 복사, worker 저장과 processed fallback을
소유한다. 카메라 컴포넌트는 RequestID가 현재 `PendingCapture`와 일치하는 결과만 처리한다.
자동 테스트에서는 실제 viewport 대신 성공·실패를 제어할 수 있는 fake backend를 사용한다.

### 10.2 게임 스레드 규칙

```text
Game thread
→ 대상 및 Subsystem 조회
→ screenshot 요청
→ 픽셀 배열 소유 복사

Worker thread
→ PNG 압축
→ 디렉터리 및 파일 쓰기

Game thread
→ RequestID 확인
→ RegisterCapturedPhoto
→ 성공·실패 이벤트 전달
```

UObject, World, Subsystem, delegate 및 `PendingCapture` 접근은 game thread에서만 한다.

## 11. 파일별 변경 범위

| 파일 | 변경 내용 |
|---|---|
| `BalhwajeomEvidenceTypes.h` | 대상 정보를 ID와 현재 상태 스냅숏으로 변경 |
| `BalhwajeomCameraTargetInterface.h` | 기존 성공 통지 제거, 핵심 조회 함수 유지 |
| `BalhwajeomPhotoCameraComponent.h/.cpp` | Subsystem 판정·이미지 저장·record 등록·pending 처리 |
| `BalhwajeomEvidenceCameraHUD.h/.cpp` | 기존 액터 수집 bool 의존 제거; 세부 연출은 후속 |
| `BalhwajeomCameraCharacter.h/.cpp` | 기존 자체 수집 목록 API 제거 또는 단계적 폐기 |
| `BalhwajeomEvidenceActor.h/.cpp` | 담당자 2가 새 인터페이스 계약 구현 |
| `BalhwajeomInvestigationSubsystem.h/.cpp` | `GetPhotoDefinition` 반영 완료, 추가 변경 없음 |

최종 제거 대상:

```text
FBalhwajeomEvidenceData
CollectedEvidence
CapturedFocusTargets
AddEvidence
HasEvidence
GetCollectedEvidence
EvidenceData.bAlreadyCollected
MarkAsCollected 호출
NotifyCameraCaptureSucceeded
```

블루프린트가 기존 함수를 참조한다면 Deprecated 처리 후 사용처를 이전하고 제거한다. 이전
중에도 기존 배열이나 bool을 촬영 여부 판정에 사용하지 않는다.

### 11.1 호환성 전환 규칙

인터페이스와 USTRUCT의 기존 필드를 첫 변경에서 바로 삭제하지 않는다.

```text
1단계: 새 ID/상태 필드 추가, 기존 필드 유지 및 Deprecated 표시
2단계: 담당자 2 EvidenceActor가 새 필드를 채우도록 변경
3단계: 카메라가 새 resolved target 경로만 사용
4단계: Blueprint Reference Viewer 및 전체 Blueprint Compile 확인
5단계: 기존 C++/Blueprint 사용처가 0개일 때 레거시 필드와 함수 삭제
```

호환 기간의 legacy 값은 표시 또는 촬영 판정의 fallback으로 사용하지 않는다. 새 ID가
유효하지 않으면 명시적으로 `InvalidTargetSnapshot`을 반환한다.

## 12. 담당자 2와의 계약

담당자 2의 EvidenceActor는 다음을 보장한다.

```text
BeginPlay에서 RegisterEvidenceActor 성공
EvidenceInstanceID가 유효하고 배치 인스턴스별로 고유함
RequestCameraTargetInfo가 현재 ObjectID와 CurrentStateID를 사용함
현재 EvidenceStateDefinition의 PhotoID와 촬영 설정을 반환함
RequestCameraFocusLocation이 CameraFocusPoint 위치를 반환함
RequestCameraFramingComponent가 실제 증거 메시를 반환함
```

담당자 3은 다음을 하지 않는다.

```text
ABalhwajeomEvidenceActor의 실제 구현 수정
EvidenceActor.CurrentStateID 직접 변경
MarkAsCollected 호출
액터 내부에 촬영 완료 bool 기록
```

## 13. 테스트 계획

### 13.1 자동 테스트

- 캡처 불가 상태는 이미지 저장을 요청하지 않는다.
- 잘못된 인스턴스·오브젝트·상태·사진 조합은 거부된다.
- 같은 `PhotoID`는 다른 액터에서도 두 번째 촬영이 차단된다.
- 같은 액터가 다른 `PhotoID` 상태로 변경되면 새로 촬영할 수 있다.
- pending 중 반복 입력은 screenshot을 한 번만 요청한다.
- 다른 시스템의 screenshot 요청을 덮어쓰지 않는다.
- 픽셀 콜백 없이 processed 이벤트만 도착해도 pending이 해제된다.
- 빈 픽셀과 파일 쓰기 실패는 record를 만들지 않는다.
- 등록 실패 시 이번 촬영으로 만든 파일만 삭제한다.
- 등록 성공 시 `ImageRelativePath`로 실제 PNG를 찾을 수 있다.
- 컴포넌트 파괴 시 screenshot delegate가 남지 않는다.
- 다른 screenshot delegate 구독자를 제거하지 않는다.

### 13.2 플레이 검증

- 카메라 진입·종료, WASD 이동, 회전, 줌과 DOF가 유지된다.
- 초점 거리, 가시성, 중앙점과 화면 70% 포함 판정이 유지된다.
- 같은 `PhotoID`를 다시 찍을 수 없다.
- 상태 변경 후 새 `PhotoID`를 촬영할 수 있다.
- 저장된 이미지에 HUD와 카메라 연출이 포함되지 않는다.
- 카메라 및 후속 3인칭 표시가 같은 Subsystem 상태를 사용한다.

## 14. 구현 순서

### 단계 0: 기준선 고정

- 현재 에디터 빌드와 Investigation 자동 테스트 통과를 확인한다.
- 담당자 2가 수정 중인 EvidenceActor 파일과 Blueprint 범위를 확인한다.

통과 조건: 공통 Subsystem과 `GetPhotoDefinition()`이 포함된 기준선이 성공한다.

### 단계 1: 호환 타입과 인터페이스

- `FBalhwajeomCameraTargetInfo`에 새 필드를 추가한다.
- 기존 필드와 함수는 Deprecated 상태로 남긴다.
- `FResolvedPhotoTarget`과 순수 검증 함수를 추가한다.

통과 조건: 담당자 2 변경 없이 기존 프로젝트가 빌드되고 검증 함수 단위 테스트가 통과한다.

### 단계 2: EvidenceActor 계약 연결

- 담당자 2가 actor 등록 및 새 target snapshot 구현을 반영한다.
- 카메라는 DataTable 기준으로 snapshot을 정규화한다.

통과 조건: 배치 actor의 ID가 유효하며 상태 변경 후 resolved `PhotoID`가 바뀐다.

### 단계 3: Subsystem 촬영 전환

- 기존 자체 배열 대신 `HasCapturedPhoto()`와 `RegisterCapturedPhoto()`를 사용한다.
- `PendingCapture`와 카메라 전용 결과 enum을 연결한다.
- 이미지 저장 전까지는 테스트 backend로 트랜잭션을 검증한다.

통과 조건: 중복 PhotoID, 다른 상태 PhotoID, 등록 거부와 pending 반복 입력 테스트가 통과한다.

### 단계 4: 이미지 백엔드

- viewport screenshot backend와 worker PNG 저장을 연결한다.
- captured/processed delegate 수명과 전역 요청 충돌을 처리한다.
- 저장 후 등록 실패 시 파일 보상을 연결한다.

통과 조건: PIE와 패키지 실행에서 PNG 생성, UI 제외, 실패 정리와 hitch 허용 기준을 통과한다.

### 단계 5: 레거시 제거

- C++와 Blueprint 사용처를 전부 이전한다.
- 기존 수집 배열, actor bool과 성공 통지를 제거한다.

통과 조건: Reference Viewer상 사용처가 없고 전체 Blueprint Compile, C++ 빌드 및 자동
테스트가 성공한다.

### 단계 6: 후속 연출

- 2D/3D 카메라 연출, 세부 HUD와 태블릿 갤러리를 별도 설계로 연결한다.

## 15. 완료 조건

- 촬영 완료 상태의 단일 원천이 `InvestigationSubsystem`이다.
- 카메라는 `PhotoID` 기준으로 중복 촬영을 차단한다.
- 촬영 당시의 인스턴스·오브젝트·상태·사진 ID가 record에 보존된다.
- 성공한 record에는 실제로 열 수 있는 이미지 상대 경로가 있다.
- 실패한 촬영은 record와 고아 이미지 파일을 남기지 않는다.
- 상태가 다른 `PhotoID`로 변경되면 같은 액터를 다시 촬영할 수 있다.
- 사진 분석용 `SentenceDefinition`은 카메라와 결합되지 않는다.
- 사진 촬영에서 키워드를 획득하거나 기록하지 않는다.
- 연출 방식과 무관하게 촬영 핵심 흐름을 재사용할 수 있다.

## 16. 위험 점검 결과

### 16.1 높은 위험

| 위험 | 발생 가능성 | 영향 | 대응 |
|---|---:|---:|---|
| 인터페이스 구조 변경 즉시 기존 EvidenceActor가 컴파일되지 않음 | 높음 | 높음 | 담당자 2 변경과 같은 통합 단위로 반영하거나 호환 필드를 먼저 추가한 뒤 단계적으로 이전 |
| 기존 블루프린트가 삭제 예정 필드·함수를 참조함 | 높음 | 높음 | `FBalhwajeomEvidenceData`, `GetCollectedEvidence` 등을 즉시 삭제하지 말고 Deprecated 처리 후 Reference Viewer와 BP compile로 사용처 확인 |
| viewport screenshot 전역 요청이 다른 screenshot 기능과 충돌함 | 중간 | 높음 | 전역 요청 여부 검사, 두 delegate handle의 개별 해제, 단일 pending, 충돌이 실제 발생하면 SceneCapture 방식으로 전환 |
| `bEnableEvidenceFocusSystem`이 기존 Blueprint에서 false로 남음 | 높음 | 높음 | 담당 카메라 Blueprint 값을 명시적으로 이전하고 legacy 촬영 경로 제거 전 플레이 검증 |
| EvidenceActor가 Subsystem에 등록되지 않았거나 GUID가 중복됨 | 중간 | 높음 | BeginPlay 등록 실패 로그, 유효·고유 GUID 에디터 검증, 촬영 전 ID 검증 |

### 16.2 중간 위험

| 위험 | 발생 가능성 | 영향 | 대응 |
|---|---:|---:|---|
| TargetInfo가 DataTable 촬영 값을 복사하면서 값이 어긋남 | 중간 | 중간 | DataTable의 `FEvidenceStateDefinition`을 최종 권위로 사용하고 촬영 순간 일치 검사; 장기적으로 TargetInfo를 ID 전용으로 축소 검토 |
| 비동기 저장 한 프레임 사이 상태가 변경되어 등록이 거부됨 | 낮음 | 중간 | 현재 Subsystem 계약대로 실패 처리하고 파일 삭제; 빈번하면 캡처 예약 API를 공통 계층에 별도 설계 |
| Canvas HUD가 저장 이미지에 포함됨 | 중간 | 중간 | PIE와 패키지에서 픽셀 검증; HUD frame suppression 또는 SceneCapture fallback |
| PNG 압축으로 프레임 hitch 발생 | 중간 | 중간 | 픽셀 소유 복사 후 worker thread 압축·저장, game thread에서만 UObject 접근 |
| 세션 폴더가 실행마다 누적됨 | 높음 | 중간 | SaveGame 도입 전에는 시작 시 이전 세션 폴더 정리; SaveGame 도입 시 보존 정책 재설계 |
| 정규화된 `PhotoID` 파일명이 충돌함 | 낮음 | 중간 | 파일명에 `RequestID` 포함 |

### 16.3 낮은 위험과 후속 범위

| 위험 | 대응 시점 |
|---|---|
| `CaptureSound.LoadSynchronous()`로 순간 hitch | 사진별 사운드를 실제 사용할 때 preload 또는 async load 설계 |
| `bViewedInTablet`을 true로 바꿀 API가 없음 | 태블릿 갤러리 구현 시 공통 API 추가 |
| 촬영 목록 조회 API가 없음 | 태블릿 갤러리 구현 시 추가 |
| 저장 이미지와 SaveGame 수명 불일치 | SaveGame 설계 시 record와 파일의 원자적 보존·삭제 정책 추가 |
| 화면 투영 bounds가 회전·오목 메시에서 부정확함 | 기존 프레이밍 판정 회귀 테스트 후 필요할 때 전용 framing volume 도입 |

## 17. 전제 조건

- 현재 게임은 단일 로컬 플레이어 세션을 기준으로 한다.
- `UGameInstanceSubsystem`의 `PhotoID` 중복 상태도 플레이어 한 명의 상태로 해석한다.
- 멀티플레이 또는 로컬 분할 화면을 지원하면 조사 상태의 소유 위치와 전역 screenshot
  사용을 다시 설계해야 한다.
- 실제 Investigation DataTable에 촬영 가능한 상태와 대응 `PhotoDefinition` 행이 입력되어야
  통합 플레이 테스트가 가능하다.
- 2D/3D 연출을 미루더라도 저장 이미지에서 연출 요소가 제외된다는 결과 조건은 유지한다.
