# 침대 기억 앰비언스 시스템 최종 설계 v3

작성 기준일: 2026-09-09  
대상 프로젝트: Balhwajeom / Unreal Engine 5.7

## 1. 최종 경험 흐름

```text
침대 근처에서 [F] 앉기
→ 플레이어 입력 잠금
→ 침대의 SeatedCamera로 전환 시작
→ 플레이어를 PlayerAnchor에 즉시 배치하고 Anchor 방향으로 침대를 정면으로 바라봄
→ 카메라 전환 중 플레이어가 제자리에서 180도 회전해 침대 반대 방향을 바라봄
→ 지정된 착석 애니메이션이 있으면 재생
→ 애니메이션이 없으면 해당 위치에 서 있는 상태 유지
→ 침대 전용 BGM 페이드인
→ 1~2초 정적
→ 가족 음성을 방 안의 서로 다른 위치에서 한 개씩 재생
→ 음성 사이에 불규칙한 정적
→ 같은 콜라이더 영역에서 [F]를 다시 눌러 일어나기
→ BGM/음성 페이드아웃
→ 퇴장 애니메이션이 있으면 재생
→ ExitAnchor로 이동한 뒤 탐색 카메라와 입력 복원
```

이 이벤트는 단서나 정답을 주는 장면이 아니다. 플레이어가 이미 조사한 가족의 음성을 공간 안에서 다시 듣는 선택형 체류 장면이다.

## 2. 핵심 설계 규칙

1. 위치 이동과 카메라 전환은 항상 동작한다.
2. 착석·기립 애니메이션은 선택 사항이다. Blueprint의 Montage 슬롯이 비어 있어도 기능 전체가 동작해야 한다.
3. 착석 애니메이션이 없으면 플레이어는 `PlayerAnchor` 위치에 서 있는 상태로 감상한다.
4. 전용 BGM은 침대 이벤트가 직접 재생하고 종료한다.
5. 가족 음성 후보는 현재 수집한 사진에서 실시간으로 만든다. 사진 정의의 `StoryVoice`가 한 번에 하나씩 재생되며, 한 셔플 순회 안에서는 반복하지 않는다.
6. 가족 음성 사이의 침묵도 연출의 일부로 취급한다.
7. 각 음성은 책상, 문, 침대, 복도처럼 기억과 연결된 방향에서 들린다.
8. 언제든 나갈 수 있으며, 나갈 때 카메라·입력·HUD·오디오가 모두 원래 상태로 복원되어야 한다.
9. 조사 진행도, 키워드, 사진, 진술서 상태는 변경하지 않는다.
10. 일반 HUD는 숨기되 사용자가 자막을 켰다면 자막은 유지한다.
11. `InteractionCollision`에 들어오면 침대가 고우선순위 F 입력을 소유하고, 같은 F를 진입/퇴장 토글로 처리한다.

## 3. 액터 Blueprint 구성

정식 Blueprint 이름은 `BP_BedMemory`로 한다. 기반 C++ Actor는 `ABedMemoryActor`로 둔다.

```text
BP_BedMemory
└─ SceneRoot
   ├─ BedMesh                 침대 메시 또는 침대 기준점
   ├─ InteractionCollision    플레이어 초점/상호작용 충돌
   ├─ InspectionComponent     기존 거리·F 상호작용 시스템 연결
   ├─ InteractionWidget       [F] 앉기 표시
   ├─ PlayerAnchor            진입 후 플레이어 위치와 방향
   ├─ ExitAnchor              일어난 뒤 플레이어 위치와 방향
   ├─ SeatedCamera            감상 중 카메라
   ├─ VoiceOrigin             음성 위치 누락 시 기본 재생점
   ├─ Voice_Desk              책상 방향 음성 재생점
   ├─ Voice_Door              문 방향 음성 재생점
   ├─ Voice_Bed               침대 가까운 음성 재생점
   ├─ Voice_Hall              방 바깥 방향 음성 재생점
   ├─ Voice_Photo             사진 방향 음성 재생점
   ├─ BGMPlayer               침대 전용 BGM Audio Component
   └─ VoicePlayer             현재 가족 음성 Audio Component
```

### Blueprint에서 직접 편집할 항목

- 침대 메시와 상호작용 Collision
- `[F] 앉기`가 나타나는 범위를 정하는 `InteractionCollision` 크기
- `PlayerAnchor` 위치와 회전
- `ExitAnchor` 위치와 회전
- `SeatedCamera` 위치, 회전, FOV
- 각 Voice Anchor의 위치
- 착석/대기/기립 Montage
- 침대 전용 BGM
- 사진별 Voice Anchor 매핑(`PhotoID → EmitterID`)
- 카메라 블렌드 시간
- 음성 전후 정적 범위
- 기존 BGM을 얼마나 낮출지 정하는 Sound Mix

카메라와 Anchor는 레벨 뷰포트에서 기획자가 직접 이동할 수 있어야 한다. 코드는 특정 침대 크기나 방향을 가정하지 않는다.

## 4. 기존 상호작용 시스템 연결

현재 `UPlayerInteractionComponent`는 `UInspectionComponent`를 찾아 거리와 중앙 초점을 판정하고 `IA_Interact`를 처리한다. 침대의 라벨과 일반 상호작용 호환성은 이 부분을 재사용하되, 실제 진입/퇴장 F 입력은 `InteractionCollision`의 오버랩 동안 침대가 직접 소유한다.

`BeginOverlap`에서 로컬 플레이어에게 고우선순위 Enhanced Input Component를 Push하고 `IA_Interact`를 소비한다. 첫 F는 진입, 두 번째 F는 퇴장이다. PlayerAnchor가 콜라이더 밖에 있어도 감상 중에는 입력 컴포넌트를 유지하므로 두 번째 F가 유실되지 않는다. 퇴장 후 플레이어가 콜라이더 밖이면 입력 컴포넌트를 제거하고, 안이면 차단을 해제한 채 다음 토글을 기다린다.

`[F] 앉기` 라벨도 거리 수치가 아니라 같은 `InteractionCollision`을 기준으로 한다. `BeginOverlap`에서 표시하고 `EndOverlap`에서 즉시 숨긴다. 감상 중에는 콜라이더 안이어도 숨기며, 퇴장 완료 후 여전히 콜라이더 안이면 다시 표시한다. 표시 문자열 자체는 `InspectionComponent::NearLabel`을 계속 사용한다.

다만 침대는 조사 문장을 출력하는 대상이 아니므로 비텍스트 상호작용을 위한 공통 인터페이스를 추가한다.

```cpp
UINTERFACE(BlueprintType)
class UWorldInteractable : public UInterface
{
    GENERATED_BODY()
};

class IWorldInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool CanInteract(APawn* InteractingPawn) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    bool RequestInteraction(APawn* InteractingPawn);
};
```

`UPlayerInteractionComponent::RequestInspect()`의 처리 순서는 다음과 같이 변경한다.

```text
FocusedInspection의 Owner 확인
→ IWorldInteractable이면 RequestInteraction 호출
→ 성공하면 여기서 종료하고 OnInspectionSucceeded는 방송하지 않음
→ 인터페이스가 없으면 기존 EvidenceActor/InspectionText 경로 실행
```

따라서 기존 증거 조사와 테스트는 유지하면서 침대, 문, 의자 같은 행동형 상호작용을 이후에도 추가할 수 있다.

## 5. 상태 머신

```cpp
enum class EBedMemoryState : uint8
{
    Idle,
    AligningPlayer,
    Entering,
    PreparingAudio,
    Listening,
    Exiting,
    Restoring
};
```

| 상태 | 역할 |
|---|---|
| Idle | 콜라이더 안에서 `[F] 앉기` 가능 |
| AligningPlayer | PlayerAnchor에 배치하고 카메라 블렌드 중 180도 회전 |
| Entering | 선택적 착석 Montage 및 카메라 진입 |
| PreparingAudio | 수집 사진 조회 및 필요한 StoryVoice 비동기 로드 |
| Listening | BGM과 가족 음성 재생, 일어나기 입력 허용 |
| Exiting | 음향 정리 및 선택적 기립 Montage |
| Restoring | ExitAnchor 이동, 카메라/HUD/입력 복원 |

진입 직후 0.25초 동안 퇴장 입력을 막아 같은 키 입력의 중복 전달을 방지한다. 그 뒤에는 Listening뿐 아니라 진입 준비 상태에서도 두 번째 F로 안전하게 퇴장할 수 있다.

## 6. 플레이어 위치 처리

### 진입 순서

1. 사용자의 이동·시점 입력을 잠그고 Character Movement를 비활성화한다.
2. 플레이어를 `PlayerAnchor` 위치에 즉시 배치한다.
3. PlayerAnchor의 Yaw를 적용해 플레이어가 침대를 정면으로 바라보게 한다.
4. 침대 카메라 블렌드를 즉시 시작한다.
5. `CameraBlendInDuration` 동안 플레이어 Yaw를 부드럽게 +180도 회전한다.
6. 회전이 끝나 침대 반대 방향을 바라보면 착석 애니메이션을 시작한다.
7. 착석 애니메이션 완료 후 수집 사진 음성과 BGM 감상을 시작한다.

자동 보행, NavMesh 탐색, 접근 속도 보정은 사용하지 않는다. 플레이어가 콜라이더 어느 위치에서 F를 눌러도 동일한 PlayerAnchor와 동일한 회전 연출에서 시작한다.

### 애니메이션이 없을 때

- `EnterMontage == nullptr`: 착석 단계 완료로 즉시 간주한다.
- `SeatedIdleMontage == nullptr`: 플레이어는 PlayerAnchor에서 기본 서기 Pose를 유지한다.
- `ExitMontage == nullptr`: 퇴장 단계 완료로 즉시 간주한다.

즉, 세 Montage가 모두 비어 있어도 위치 이동, 카메라, BGM, 가족 음성, 퇴장이 정상 동작해야 한다.

### 애니메이션이 있을 때

Blueprint에서 다음 슬롯에 Montage를 지정한다.

```cpp
TSoftObjectPtr<UAnimMontage> EnterMontage;
TSoftObjectPtr<UAnimMontage> SeatedIdleMontage;
TSoftObjectPtr<UAnimMontage> ExitMontage;
```

- `EnterMontage`: 서기에서 앉기로 전환
- `SeatedIdleMontage`: 앉은 자세 반복. Loop 설정 권장
- `ExitMontage`: 앉기에서 서기로 전환

진입 완료는 고정 Delay보다 Montage 종료 Delegate 또는 `AnimNotify_BedSeatReady`를 우선 사용한다. Notify가 없거나 Montage 재생에 실패하면 Montage 길이를 기준으로 폴백하고, 그것도 알 수 없으면 즉시 Listening으로 넘어간다.

Root Motion은 기본적으로 사용하지 않는다. 월드 위치의 기준은 항상 `PlayerAnchor`다. Root Motion을 사용할 경우 Blueprint 옵션을 켜고 Montage 종료 후 PlayerAnchor에 다시 보정한다.

### 퇴장 위치

기립 처리 후 플레이어를 `ExitAnchor`로 이동한다. ExitAnchor가 막혀 있다면 작은 반경으로 Collision 안전 위치를 찾고, 실패하면 이벤트 시작 전 위치로 폴백한다.

## 7. 카메라

`SeatedCamera`는 `BP_BedMemory`의 자식 Camera Component로 두고 기획자가 자유롭게 편집한다.

권장 흐름:

```text
PlayerAnchor 정렬 시작
→ SeatedCamera로 0.4~0.7초 Blend
→ Blend 완료 시 BGM/Listening 시작
```

착석 Montage가 있다면 카메라 블렌드와 Montage를 동시에 시작하는 것을 기본으로 한다. 애니메이션이 없어도 같은 카메라 흐름을 사용한다.

감상 중 카메라는 기본적으로 고정한다. 제한적인 Look 기능은 1차 구현에서 제외하고 플레이테스트 후 추가한다.

퇴장 시 `ABalhwajeomCameraCharacter::RestoreExplorationView()`를 호출한다. 이 함수는 현재 `ActiveCameraZone`이 있으면 Zone 카메라를, 없으면 Character 카메라를 복원해야 한다. 침대 액터가 고정 카메라 Zone 내부 상태를 직접 추측해서는 안 된다.

## 8. 전용 BGM

침대 이벤트에는 전용 BGM이 들어간다.

권장 속성:

```cpp
TSoftObjectPtr<USoundBase> BedBGM;
float BGMFadeInDuration = 1.5f;
float BGMFadeOutDuration = 0.8f;
float BGMVolume = 0.7f;
TObjectPtr<USoundMix> BedSoundMix;
```

### 재생 규칙

1. SeatedCamera 블렌드가 시작되면 기존 BGM을 Sound Mix로 약 -6~-10 dB 낮춘다.
2. `BGMPlayer`에서 침대 전용 BGM을 페이드인한다.
3. 가족 음성 재생 중에도 전용 BGM은 유지하되 보이스보다 작게 들려야 한다.
4. 일어나기 입력 시 전용 BGM을 페이드아웃한다.
5. 전용 BGM 종료와 함께 기존 BGM Sound Mix를 원래 값으로 복원한다.

전용 BGM은 반복 가능한 Sound Cue 또는 MetaSound로 준비한다. 단발 SoundWave를 무한 반복한다고 코드에서 가정하지 않는다.

`BGMPlayer` Audio Component를 사용하는 이유는 언제든 정확하게 FadeOut하고 Stop할 수 있기 때문이다. `PlaySound2D`처럼 반환 제어권이 없는 방식은 사용하지 않는다.

침대 전용 BGM이 비어 있으면 가족 음성과 기존 환경음만으로 이벤트를 계속한다. BGM 누락은 진입 실패 사유가 아니다.

## 9. 가족 음성 데이터 원본

별도의 침대용 음성 Playlist나 해금 태그를 만들지 않는다. 현재 조사 시스템에 이미 다음 데이터가 있다.

```cpp
struct FPhotoDefinition
{
    FName PhotoID;
    TArray<FText> WorldStoryLines;
    TSoftObjectPtr<USoundBase> StoryVoice;
};
```

`UBalhwajeomInvestigationSubsystem`은 현재 수집된 사진을 `GetCapturedPhotos()`로 제공하고, 각 `PhotoID`의 정의는 `GetPhotoDefinition()`으로 조회할 수 있다. 침대는 이 두 API를 단일 기준으로 사용한다.

```text
GetCapturedPhotos()
→ 각 FCapturedPhotoRecord.PhotoID 확인
→ GetPhotoDefinition(PhotoID)
→ StoryVoice가 설정된 사진만 후보에 추가
→ 후보를 셔플 백으로 구성
```

침대 전용 후보 생성 함수는 다음 책임만 가진다.

```cpp
struct FBedMemoryVoiceCandidate
{
    FName PhotoID;
    TSoftObjectPtr<USoundBase> StoryVoice;
    FName EmitterID;
};

void ABedMemoryActor::BuildVoiceCandidates()
{
    VoiceCandidates.Reset();

    UGameInstance* GameInstance = GetGameInstance();
    UBalhwajeomInvestigationSubsystem* Investigation =
        GameInstance
            ? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
            : nullptr;

    if (!Investigation)
    {
        return;
    }

    TArray<FCapturedPhotoRecord> CapturedPhotos;
    Investigation->GetCapturedPhotos(CapturedPhotos);

    for (const FCapturedPhotoRecord& Record : CapturedPhotos)
    {
        FPhotoDefinition Definition;
        if (!Investigation->GetPhotoDefinition(Record.PhotoID, Definition) ||
            Definition.StoryVoice.IsNull())
        {
            continue;
        }

        FBedMemoryVoiceCandidate Candidate;
        Candidate.PhotoID = Record.PhotoID;
        Candidate.StoryVoice = Definition.StoryVoice;
        Candidate.EmitterID = ResolveEmitterID(Record.PhotoID);
        VoiceCandidates.Add(MoveTemp(Candidate));
    }
}
```

이 함수는 `UBalhwajeomInvestigationSubsystem`의 내부 `CapturedPhotos` Map이나 DataTable 포인터에 직접 접근하지 않는다. 공개 API만 사용하므로 조사 시스템의 저장 방식이 바뀌어도 침대 코드의 결합도가 낮다.

따라서 결과는 다음과 같다.

| 현재 수집한 사진 | StoryVoice 설정 | 침대에서 재생할 음성 후보 |
|---:|---:|---:|
| 0개 | 해당 없음 | 0개 |
| 1개 | 1개 | 1개 |
| 3개 | 3개 | 3개 |
| 3개 | 2개 | 2개 |

사진을 3개 모으면 음성도 3개가 나온다는 기획을 보장하려면 본편의 모든 수집 가능 사진에 `StoryVoice`를 필수로 설정해야 한다. 값이 비어 있는 사진은 침대 후보에서 안전하게 제외하고 경고 로그를 남긴다.

여기서 “사진이 없으면 아무 소리도 나지 않는다”는 것은 가족 음성이 재생되지 않는다는 의미로 정의한다. 침대 전용 BGM과 방 환경음은 그대로 유지한다. BGM까지 끄는 연출이 필요하면 별도 Blueprint 옵션으로 둘 수 있지만 기본값은 아니다.

### 사진과 공간 위치 연결

음원 자체는 `FPhotoDefinition::StoryVoice`에서 가져오지만, 어느 위치에서 들릴지는 방마다 달라질 수 있으므로 사진 DataTable에 공간 위치를 넣지 않는다. `BP_BedMemory`가 다음 매핑을 가진다.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Voice")
TMap<FName, FName> PhotoEmitterMap;
// PhotoID → EmitterID
```

예시:

```text
PHOTO_DESK_FAMILY  → Desk
PHOTO_DOOR_SCOLD   → Door
PHOTO_BED_SIBLINGS → Bed
PHOTO_FAMILY_CALL  → Hall
```

매핑이 없는 사진은 `VoiceOrigin`에서 재생한다. 이 구조는 사진과 음원의 연결을 중복 저장하지 않으면서 침대 Blueprint에서 공간 연출만 편집하게 한다.

### 세션 중 새 사진이 생기는 경우

침대에 앉아 있는 동안에는 사진을 촬영할 수 없으므로 진입 시점에 후보 Snapshot을 만든다. 일어난 뒤 새 사진을 수집하고 다시 앉으면 새로운 사진의 `StoryVoice`가 다음 후보 목록에 포함된다.

`OnPhotoCaptured` 이벤트를 침대가 계속 구독할 필요는 없다. 매 진입 시 `GetCapturedPhotos()`를 다시 호출하는 편이 단순하며 저장 복원 사진과 방금 촬영한 사진을 같은 방식으로 처리한다.

### 저장 사진 복원과 초기화 순서

`UBalhwajeomInvestigationSubsystem::Initialize()`는 다음 순서로 실행된다.

```text
DataTable 로드
→ DataTable 검증
→ 기본 키워드 초기화
→ LoadPersistentPhotoGallery()
```

따라서 월드의 침대 Actor가 `BeginPlay`한 뒤 플레이어가 상호작용하는 시점에는 유효한 저장 사진이 이미 `CapturedPhotos`에 들어 있다. 별도의 침대 SaveGame이나 로드 완료 대기 이벤트는 만들지 않는다.

저장 복원 과정에서 사진 정의가 없거나 이미지 파일이 유효하지 않은 기록은 Subsystem이 이미 제외한다. 침대는 Subsystem이 반환한 사진을 다시 파일 경로로 검증하지 않는다. `CapturedPhotos`를 신뢰 가능한 단일 기준으로 사용한다.

### Soft Object 로드 정책

`StoryVoice`는 `TSoftObjectPtr<USoundBase>`이므로 후보 구성과 실제 재생 사이에 로드 단계가 필요하다.

1. `PreparingAudio`에서 후보들의 유효한 Soft Object Path를 모은다.
2. 중복 경로를 제거한다.
3. `FStreamableManager::RequestAsyncLoad`로 한 번에 요청한다.
4. 로드가 끝난 후보만 셔플 백에 넣고 Listening을 시작한다.
5. 로드 실패 항목은 경고를 남기고 제외한다.
6. 플레이어가 로드 중 일어나면 Handle을 취소하거나 완료 Callback에서 현재 상태를 검사해 아무 작업도 하지 않는다.

동기 `LoadSynchronous()`를 음성 재생 직전에 호출하면 짧은 음성 사이에서 프레임 끊김이 보일 수 있으므로 사용하지 않는다. BGM은 침대 Blueprint의 Audio Component가 참조하여 미리 로드되게 한다.

## 10. 가족 음성 재생 방식

### 기본 타이밍

| 항목 | 기본값 |
|---|---:|
| 첫 음성 전 정적 | 1.0~2.0초 |
| 일반 음성 사이 정적 | 1.0~4.0초 |
| 긴 정적 확률 | 20% |
| 긴 정적 | 4.0~7.0초 |

### 무중복 셔플 백

1. 진입 시점에 수집된 사진 중 `StoryVoice`가 있는 항목을 수집한다.
2. 셔플 백이 비었으면 Fisher–Yates로 새 순서를 만든다.
3. 음성이 2개 이상이고 새 순서의 첫 항목이 직전 재생 음성과 같으면 다른 항목과 교환한다.
4. 한 사진을 꺼내 `PhotoEmitterMap`에서 `EmitterID`를 찾고 그 위치에서 `StoryVoice`를 재생한다.
5. `UAudioComponent::OnAudioFinished`가 호출된 뒤 다음 정적을 시작한다.

재생 길이를 `GetDuration()`으로 예약하지 않는다. 음원 스트리밍이나 에셋 교체에도 정확하게 이어지도록 실제 재생 완료 이벤트를 사용한다.

플레이어가 일어났다가 바로 다시 앉더라도 방금 들은 사진 음성이 첫 음성으로 반복되지 않게 `LastPlayedPhotoID`는 액터가 살아 있는 동안 유지한다. 후보 Snapshot은 재진입할 때 다시 만들기 때문에 그 사이 새로 수집한 사진도 반영된다.

### 음성 개수별 처리

- 수집 사진 0개: 전용 BGM과 환경음만 재생하고 가족 음성은 재생하지 않음
- 수집 사진은 있지만 유효한 StoryVoice가 0개: 위와 동일하게 처리하고 설정 경고 출력
- 1개: 재생 후 반드시 긴 정적 사용
- 2개 이상: 연속 중복 금지 및 한 바퀴 무중복
- Sound 로드 실패: 해당 항목을 건너뛰고 정적 후 다음 항목 시도

## 11. 공간 음향

`EmitterID`를 이름이 같은 Voice Anchor Scene Component에 연결한다.

| EmitterID | 권장 위치 | 예시 |
|---|---|---|
| Bed | 카메라 가까운 침대 쪽 | 낮은 목소리, 가까운 대화 |
| Desk | 책상 부근 | 책상 조사 보이스 |
| Door | 방문 부근 | 문에서 부르는 소리 |
| Hall | 방 바깥 방향 | 가족의 언쟁, 식사 호출 |
| Photo | 사진 부근 | 사진과 연결된 대화 |

음성 에셋은 가능하면 mono로 준비하고 전용 Attenuation 에셋을 적용한다. Stereo 음원은 위치 재생을 해도 기대한 좌우 방향감이 나오지 않을 수 있으므로 자산 검수 항목에 포함한다.

감쇠는 실제 거리보다 완만하게 설정한다. 어디서 재생되든 대사는 알아들을 수 있어야 하며 방향감만 은근하게 느껴져야 한다.

EmitterID를 찾지 못하면 `VoiceOrigin`에서 재생하고 경고 로그를 한 번 남긴다.

## 12. 입력과 UI

콜라이더 진입 시 고우선순위 Input Component를 Push하고 `IA_Interact`를 소비한다. 감상 상태에서는 하위 Gameplay Input도 차단한다.

감상 중 허용 입력:

- `F`: 일어나기

차단 입력:

- 이동, 달리기, 점프
- 시점 회전
- 사진 카메라, 촬영
- 태블릿
- 다른 조사 상호작용

표시 정책:

```text
진입 전: [F] 앉기
진입 직후 1.5~2초: [F] 일어나기
그 이후: 일반 UI 없음
```

자막은 UI 숨김 대상에서 제외한다.

사진 카메라에 있는 EvidenceActor 전용 라벨 숨김 코드는 공용 Presentation Suppression API로 분리하는 것이 좋다. `UPlayerInteractionComponent::SetPresentationSuppressed(bool)`를 두고 Evidence와 침대의 Label Presenter가 함께 구독하도록 한다.

## 13. 진입과 퇴장 조건

### 진입 가능

- 침대 상태가 Idle
- 플레이어가 `InteractionCollision` 안에 있음
- 태블릿이 닫혀 있음
- 사진 카메라 모드와 전환이 종료됨
- 다른 강제 연출 상태가 아님

### 퇴장 정리 순서

1. 다음 음성 Timer 해제
2. 현재 가족 음성 0.15~0.25초 FadeOut
3. 전용 BGM FadeOut
4. 기존 BGM Sound Mix 복원
5. `SeatedIdleMontage` 중지
6. `ExitMontage`가 있으면 재생, 없으면 즉시 진행
7. 플레이어를 ExitAnchor 또는 안전 위치로 이동
8. 탐색 카메라 복원
9. HUD와 월드 라벨 복원
10. 입력과 Character Movement 복원
11. 상태를 Idle로 변경

`EndPlay`, 레벨 전환, Pawn 교체, 액터 파괴에서도 동일한 정리 루틴을 사용한다. Timer, Audio Component, Sound Mix, Input Component는 세션마다 정확히 한 번 적용하고 한 번 해제한다.

## 14. 에셋 구조

```text
Content/Balhwajeom/Gameplay/Interaction/BedMemory/
  BP_BedMemory

Content/Balhwajeom/Audio/Music/
  SC_BedMemory_BGM
  MX_BedMemory

Content/Balhwajeom/Audio/SFX/MemoryVoice/
  ATT_MemoryVoice
  DT_Photos의 StoryVoice가 참조하는 기존 가족 음성 에셋

Content/Balhwajeom/Characters/Player/Animations/
  AM_Bed_Enter       선택
  AM_Bed_Idle        선택
  AM_Bed_Exit        선택
```

## 15. 테스트 기준

### 필수 기능 테스트

1. Montage 세 개가 모두 비어 있어도 전체 기능이 동작한다.
2. 애니메이션이 없으면 플레이어가 PlayerAnchor에 서 있는 상태로 유지된다.
3. Montage가 있을 때 진입, 앉은 대기, 퇴장 순서가 올바르다.
4. PlayerAnchor와 SeatedCamera를 회전시켜도 코드 수정 없이 올바르게 동작한다.
5. BGM이 페이드인되고 퇴장 시 완전히 정리된다.
6. 기존 BGM은 감상 중 낮아지고 퇴장 후 원래 음량으로 돌아온다.
7. 가족 음성이 한 번에 하나만 재생된다.
8. 한 셔플 순회 안에서 같은 PhotoID의 음성이 반복되지 않는다.
9. 음성 사이에 일반 정적과 긴 정적이 실제로 발생한다.
10. 각 음성이 지정된 Anchor 방향에서 들린다.
11. 어느 타이밍에 일어나도 추가 음성이 재생되지 않는다.
12. FixedCameraZone 안에서도 퇴장 후 올바른 탐색 카메라로 복귀한다.
13. 태블릿과 사진 카메라가 감상 중 열리지 않는다.
14. HUD는 사라지지만 자막은 유지된다.
15. 20회 반복 후에도 입력 잠금, Sound Mix, Audio Component, Timer가 남지 않는다.
16. `GetCapturedPhotos()`가 0개면 가족 음성 후보도 정확히 0개다.
17. 수집 사진 3개 모두에 `StoryVoice`가 있으면 후보도 정확히 3개다.
18. 수집하지 않은 사진의 `StoryVoice`는 후보에 들어오지 않는다.
19. `StoryVoice`가 비어 있거나 로드에 실패한 사진은 건너뛴다.
20. 새 사진을 촬영한 뒤 재진입하면 후보 수가 갱신된다.
21. 저장 후 재실행해 복원된 사진도 신규 촬영 사진과 동일하게 후보가 된다.
22. PreparingAudio 중 퇴장해도 늦게 도착한 비동기 Callback이 음성을 재생하지 않는다.

### 감성 테스트

- 첫 음성이 너무 빨리 시작하지 않는가.
- 전용 BGM이 가족 음성을 감정적으로 강요하거나 덮지 않는가.
- 정적이 빈 시간보다 공간을 느끼는 시간으로 받아들여지는가.
- 음성 방향이 자연스러운가, 플레이어가 소리를 찾느라 산만해지는가.
- 카메라에 웜톤 햇빛과 생활 흔적이 충분히 들어오는가.
- 플레이어가 서 있어도 카메라 구도상 어색하지 않은가.

애니메이션 없는 상태도 정식 지원하므로 마지막 항목이 중요하다. `SeatedCamera`에서는 가능하면 플레이어 하체나 서 있는 실루엣이 보이지 않게 구도를 잡는다.

## 16. 구현 순서

### 1단계 — 액터와 상호작용

- `IWorldInteractable` 추가
- `UPlayerInteractionComponent`의 비텍스트 상호작용 경로 추가
- `ABedMemoryActor`와 상태 머신 구현
- PlayerAnchor, ExitAnchor, SeatedCamera 구현
- 애니메이션 없는 진입/퇴장부터 검증

### 2단계 — 선택적 애니메이션

- Enter/Idle/Exit Montage 슬롯 추가
- Montage 종료 Delegate와 Notify 연결
- Montage가 비었거나 실패했을 때 폴백 검증
- Root Motion 미사용 기준 위치 보정

### 3단계 — BGM과 가족 음성

- BGMPlayer와 Sound Mix 적용
- `UBalhwajeomInvestigationSubsystem` 조회 및 null-safe 실패 처리
- `GetCapturedPhotos()`와 `GetPhotoDefinition()` 기반 후보 Snapshot 구현
- `PhotoEmitterMap`과 사진별 StoryVoice 연결
- StoryVoice 비동기 일괄 로드와 취소 처리
- 셔플 백과 정적 Timer 구현
- Voice Anchor 공간 재생과 Attenuation 연결

### 4단계 — UI와 복원

- HUD/월드 라벨 억제와 복원
- 자막 유지 검증
- 사진 0개, StoryVoice 누락, 사진 추가 후 재진입 검증

### 5단계 — 본편 튜닝

- 카메라와 햇빛 구도 조정
- 전용 BGM 음량과 페이드 조정
- 음성별 위치, 음량, 정적 길이 조정
- 반복 플레이와 패키지 환경 회귀 테스트

## 17. 1차 완료 조건

- `BP_BedMemory`에서 침대, PlayerAnchor, ExitAnchor, SeatedCamera 위치를 직접 편집할 수 있다.
- 애니메이션 슬롯이 비어 있어도 플레이어가 지정 위치로 이동해 서 있는 상태로 감상할 수 있다.
- Montage를 지정하면 같은 코드에서 착석, 대기, 기립 애니메이션이 선택적으로 실행된다.
- 전용 BGM과 가족 음성이 함께 재생되며 보이스가 묻히지 않는다.
- 현재 수집한 사진의 `StoryVoice`만 공간별 Anchor에서 한 바퀴 무중복으로 재생된다.
- 저장에서 복원된 사진과 현재 세션에서 새로 촬영한 사진이 같은 규칙으로 반영된다.
- 플레이어가 언제든 안전하게 나가고 모든 상태가 원복된다.
- 조사 진행도에는 어떤 변화도 생기지 않는다.
