# 침대 기억 앰비언스 시스템 설계

작성 기준일: 2026-09-09  
대상 프로젝트: Balhwajeom / Unreal Engine 5.7

## 1. 목적

플레이어가 침대에서 `F`를 눌러 앉으면 조사 중 개별적으로 들었던 가족 음성이 방 안의 서로 다른 위치에서 느슨한 간격으로 재생된다. 이 시간에는 단서를 얻거나 문제를 풀지 않는다. 플레이어가 원할 때까지 머물며, 다시 입력하면 즉시 탐색으로 돌아간다.

이 기능의 목적은 신규 서사를 제공하는 것이 아니라 이미 조사한 공간을 생활공간으로 다시 느끼게 하는 것이다.

## 2. 플레이 경험 원칙

1. **선택형이어야 한다.** 자동 진입, 최소 감상 시간, 완료 보상은 두지 않는다.
2. **침묵을 보존한다.** 보이스 사이의 정적이 환경음, 음악, 햇빛을 느끼는 시간이다.
3. **반복 티를 줄인다.** 한 셔플 백이 소진될 때까지 같은 음성을 다시 재생하지 않는다.
4. **공간감을 준다.** 각 보이스는 기억과 연결된 위치에서 재생하되 방향성이 과장되지 않게 한다.
5. **조사 시스템과 분리한다.** 이 이벤트는 증거, 키워드, 사진, 문장 진행도를 변경하지 않는다.
6. **설명하지 않는다.** 진입 전에는 `[F] 앉기`, 진입 후에는 짧은 `[F] 일어나기`만 허용한다. 별도의 튜토리얼이나 감상 완료 문구는 없다.
7. **접근성 UI는 남긴다.** 일반 HUD는 숨겨도 사용자가 자막을 켠 경우 자막은 유지한다.

## 3. 현재 프로젝트 기준 판단

현재 `UPlayerInteractionComponent`는 다음 흐름을 제공한다.

```text
UInspectionComponent 수집
→ 거리 상태 계산
→ 화면 중앙 Visibility Trace로 하나를 Focus
→ IA_Interact Started
→ EvidenceActor 조사 또는 일반 InspectionText 출력
```

침대도 `UInspectionComponent`를 사용하면 기존 거리 판정, 중앙 조준, `IA_Interact`, 거리 라벨을 그대로 재사용할 수 있다. 그러나 침대 진입은 텍스트 조사 성공이 아니므로 `OnInspectionSucceeded`를 발생시키면 안 된다.

또한 현재 카메라 캐릭터에는 고정 카메라 존, 사진 카메라, 태블릿이 각각 카메라 또는 입력을 제어하는 코드가 있다. 침대 시퀀스는 이들과 동시에 활성화되지 않도록 명시적인 진입 조건과 복원 순서가 필요하다.

추적된 `Content/Balhwajeom/Audio` 하위에는 아직 실제 음원 에셋이 없다. 따라서 시스템은 음원 슬롯이 비어 있어도 안전하게 동작해야 하며, 실제 보이스와 믹스 에셋 연결은 콘텐츠 통합 단계에서 수행한다.

## 4. 범위

### 이번 기능에 포함

- 침대 `F` 상호작용과 진입 가능 여부 판정
- 앉은 시점 카메라로 블렌드
- 이동, 점프, 시점 회전, 사진 카메라, 태블릿, 일반 조사 입력 차단
- 일반 HUD와 월드 조사 라벨 숨김
- 무중복 셔플 재생
- 최초 지연, 일반 정적, 긴 정적
- 보이스별 공간 재생 위치
- BGM/환경음/보이스 믹싱
- 언제든 일어나기와 원래 탐색 상태 복원
- 자막 유지

### 이번 기능에서 제외

- 신규 보이스 녹음 또는 신규 대사 작성
- 단서, 키워드, 사진, 업적, 완료 보상 지급
- 강제 감상 시간 또는 자동 종료
- 세이브 파일에 현재 셔플 순서 저장
- 전신 착석 애니메이션과 정교한 IK
- 이벤트 전용 햇빛 변화 연출

햇빛은 기본 레벨 라이팅으로 존재하게 두는 것을 1차안으로 한다. 이벤트 진입 때 색온도나 노출을 크게 바꾸면 플레이어가 같은 공간을 새롭게 받아들이는 효과보다 연출 장치 자체가 먼저 보일 수 있다.

## 5. 권장 구조

```text
UPlayerInteractionComponent
  └─ Focus된 Actor가 IWorldInteractable 구현 시 일반 조사보다 먼저 호출
       └─ ABedMemoryActor
            ├─ UInspectionComponent       : 거리/초점/라벨용
            ├─ UCameraComponent           : 앉은 시점
            ├─ USceneComponent            : SitViewAnchor
            ├─ USceneComponent[]          : Desk, Door, Bed, Hall 등 음원 위치
            ├─ UAudioComponent            : 현재 보이스 한 개 재생
            └─ UBedMemoryPlaylistDataAsset: 음성 목록/타이밍/믹스 정의
```

### 5.1 `IWorldInteractable`

침대 전용 분기를 `UPlayerInteractionComponent`에 하드코딩하지 않기 위한 얇은 인터페이스다.

권장 API:

```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
bool CanInteract(APawn* InteractingPawn) const;

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
bool RequestInteraction(APawn* InteractingPawn);
```

`UPlayerInteractionComponent::RequestInspect()`는 Focus Actor가 이 인터페이스를 구현하면 `RequestInteraction`을 먼저 호출하고, 처리에 성공한 경우 `OnInspectionSucceeded`를 방송하지 않는다. 기존 EvidenceActor와 일반 조사 경로는 그대로 유지한다.

이 변경은 향후 문 열기, 의자 앉기처럼 텍스트 조사가 아닌 `F` 상호작용에도 재사용할 수 있다.

### 5.2 `ABedMemoryActor`

이 액터가 한 번의 체류 세션을 소유한다. 전역 Subsystem으로 만들지 않는다. 방 하나의 배치 정보, 카메라, 공간 음원 위치가 모두 레벨 인스턴스에 종속되기 때문이다.

주요 책임:

- 진입 가능 상태 검증
- `Idle → Entering → Listening → Exiting → Idle` 상태 전환
- 카메라와 입력 상태 저장/복원
- 셔플 백 생성과 다음 보이스 선택
- 보이스 종료 후 다음 정적 타이머 시작
- Sound Mix 적용/해제
- 비정상 종료 시 정리

권장 상태 enum:

```cpp
enum class EBedMemoryState : uint8
{
    Idle,
    Entering,
    Listening,
    Exiting
};
```

상태 전환 중 중복 입력은 무시한다. `EndPlay`, 플레이어 Pawn 교체, 레벨 전환 시에는 타이머와 오디오를 중지하고 입력 및 믹스를 반드시 복원한다.

### 5.3 `UBedMemoryPlaylistDataAsset`

음성 목록과 튜닝 값을 코드에서 분리한다. 같은 음성 묶음을 테스트 맵과 본편 맵에서 재사용할 수 있다.

권장 음성 항목:

```cpp
struct FBedMemoryVoiceEntry
{
    FName VoiceID;
    TSoftObjectPtr<USoundBase> Sound;
    FName EmitterID;                 // Desk, Door, Bed, Hall 등
    FGameplayTagQuery UnlockQuery;   // 비어 있으면 항상 사용 가능
    TArray<FName> RequiredWordIDs;   // 선택 사항
};
```

권장 재생 설정:

| 항목 | 초기값 | 설명 |
|---|---:|---|
| InitialDelay | 1.0~2.0초 | 앉은 직후 첫 음성까지의 정적 |
| NormalGap | 1.0~4.0초 | 일반 보이스 사이 정적 |
| LongGapChance | 0.20 | 긴 정적을 선택할 확률 |
| LongGap | 4.0~7.0초 | 긴 정적 길이 |
| CameraBlendIn | 0.6초 | 앉은 시점 진입 |
| CameraBlendOut | 0.4초 | 탐색 시점 복귀 |
| AudioMixFade | 1.0초 | 배경 믹스 전환 |

범위 값은 `FRandomStream`으로 선택한다. 제작 중 재현이 필요하면 에디터에서 Seed를 고정할 수 있게 하고, 본편에서는 세션별 Seed를 사용한다.

## 6. 음성 해금 정책

기획 의도상 가장 적합한 기본 정책은 **플레이어가 이미 들은 음성만 재생**하는 것이다. 아직 조사하지 않은 보이스가 침대에서 먼저 나오면 침대가 생활의 재구성이 아니라 정보 선공개 장치가 된다.

권장 방식은 원래 보이스가 재생된 순간 `Story.MemoryVoice.<VoiceID>.Heard` 형태의 Gameplay Tag를 기록하고, 각 항목의 `UnlockQuery`에서 이를 검사하는 것이다. 현재 `UStoryStateSubsystem`이 Gameplay Tag Query를 지원하므로 기존 패턴과도 맞는다.

다만 본편 보이스 재생 경로가 아직 통합되지 않았다면 1차 프로토타입에서는 다음 두 모드를 둔다.

- `DiscoveredOnly`: 해금 조건을 만족한 음성만 재생하는 본편 기본값
- `AllConfigured`: 배치된 모든 음성을 재생하는 기능 검증용 값

사용 가능한 음성이 0개여도 앉기는 허용하고 환경음과 음악만 들려준다. 1개면 그 음성은 매 회차마다 다시 나올 수밖에 있으므로 긴 정적을 강제한다. 2개 이상이면 연속 중복을 금지한다.

보이스 해금 태그는 전체 진행 저장이 구현될 때 함께 저장해야 한다. 침대 내부의 셔플 순서와 현재 인덱스는 저장 대상이 아니다.

## 7. 셔플 및 타이밍 알고리즘

### 셔플 백

1. 진입 시 현재 해금된 유효 음성만 수집한다.
2. 남은 셔플 백이 비어 있으면 Fisher–Yates 방식으로 새 순서를 만든다.
3. 항목이 2개 이상이고 새 백의 첫 항목이 직전에 재생한 항목과 같으면 다른 항목과 교환한다.
4. 백의 앞 항목을 하나 꺼내 재생한다.
5. 음성 재생 완료 이벤트가 온 뒤에만 다음 정적 타이머를 건다.
6. 정적 타이머가 끝나면 2번부터 반복한다.

셔플 백과 `LastPlayedVoiceID`는 액터가 살아 있는 동안 유지한다. 플레이어가 5초 만에 일어났다가 바로 다시 앉아도 방금 들은 음성이 첫 음성으로 반복되지 않는다.

### 의사 코드

```text
BeginListening
  → Wait Random(InitialDelay)
  → PlayNextVoice

PlayNextVoice
  → Refresh eligible entries
  → Refill and shuffle bag if empty
  → Avoid last-played item at bag head
  → Resolve EmitterID to world position
  → Play one spatial voice

OnVoiceFinished
  → choose LongGap or NormalGap
  → Wait selected silence
  → PlayNextVoice
```

재생 길이를 `GetDuration()`으로 계산해 예약하지 않고 `UAudioComponent::OnAudioFinished`를 사용한다. 스트리밍, 에셋 길이 변경, 피치 변경에도 다음 타이밍이 어긋나지 않기 때문이다.

## 8. 공간 음향

`EmitterID`는 침대 액터 또는 Blueprint에 배치한 Scene Component와 연결한다.

권장 기본 배치:

| EmitterID | 위치 | 용도 |
|---|---|---|
| Bed | 앉은 카메라 가까이 | 침대 가까이에서 나눈 말, 낮은 목소리 |
| Desk | 책상 부근 | 책상 조사 보이스 |
| Door | 방문 부근 | 방문에서 부르는 소리 |
| Hall | 방 바깥 방향 | 가족 간 언쟁, 식사 호출 |
| Photo | 사진이 놓인 위치 | 사진과 연결된 회상 |

음성은 한 번에 하나만 재생한다. 보이스 에셋은 가능하면 mono로 준비하고 전용 Attenuation 에셋을 사용한다. 감쇠 거리는 실제 거리감보다 완만하게 잡아 어느 위치에서도 대사가 알아들을 수 있게 한다. 방향감은 제공하되 벽 차폐와 강한 저역 필터는 1차 범위에서 제외한다.

존재하지 않는 `EmitterID`는 침대 액터의 기본 Voice Origin으로 폴백하고 경고 로그를 한 번만 남긴다. 잘못된 위치 설정 때문에 재생 자체가 멈추면 안 된다.

## 9. 오디오 믹싱

침대 전용 `USoundMix`와 Sound Class 계층을 사용한다.

권장 목표값:

- 기억 보이스: 기준 0 dB
- BGM: 평소 대비 약 -4~-6 dB
- 실내 환경음: 평소 대비 약 -2 dB
- UI 효과음: 입력 확인에 필요한 최소 소리만 유지

진입 시 Sound Mix를 약 1초에 걸쳐 적용하고, 퇴장 시 같은 시간으로 원복한다. 환경음을 완전히 끄지 않는다. 정적 구간이 무음이 아니라 방 자체의 소리를 듣는 시간이 되어야 한다.

`PushSoundMixModifier`와 `PopSoundMixModifier` 호출은 세션당 정확히 한 번씩 짝을 맞춘다. 중복 진입, 레벨 종료, 액터 파괴에서도 Pop이 누락되지 않게 `bSoundMixApplied`를 별도로 추적한다.

## 10. 카메라와 플레이어 제어

### 진입

1. 상태가 `Idle`인지 확인한다.
2. 태블릿이 닫혀 있고 사진 카메라 모드 및 카메라 전환이 끝났는지 확인한다.
3. 캐릭터 이동 속도를 0으로 만드는 대신 Character Movement를 비활성화한다.
4. 침대 전용 고우선순위 입력 컴포넌트를 Push하고 하위 입력을 차단한다.
5. 현재 HUD 표시 상태와 커서 상태를 저장한다.
6. 일반 HUD와 월드 조사 라벨을 숨긴다.
7. 침대 액터의 `SeatedCamera`로 `SetViewTargetWithBlend`한다.
8. 블렌드가 끝난 뒤 `Listening`에 진입하고 최초 정적 타이머를 시작한다.

1차 구현에서는 플레이어 Capsule을 침대 위로 이동시키지 않는다. 카메라만 앉은 시점으로 이동하고 캐릭터는 기존 위치에 정지시킨다. 이는 Capsule 충돌, 고정 카메라 Zone overlap 변경, 침대 끼임, 퇴장 위치 탐색 문제를 피한다. 앉은 카메라에 플레이어 전신이 보여야 하는 연출이 확정되면 이후 Montage와 Sit/Exit Anchor를 추가한다.

### 체류 중 입력

- 허용: `F` 일어나기, `Esc` 일어나기(선택)
- 차단: 이동, 달리기, 점프, 시점 회전, 사진 카메라, 촬영, 태블릿, 다른 조사

마우스 시점은 1차안에서 고정한다. 제한된 Look을 허용하면 공간 음원 방향을 찾는 재미가 생길 수 있지만 앉은 카메라의 구도와 햇빛 연출이 무너질 수 있으므로 플레이테스트 후 별도 옵션으로 판단한다.

### 퇴장

1. 상태를 `Exiting`으로 바꾸고 다음 재생 타이머를 즉시 해제한다.
2. 현재 보이스를 0.15~0.25초로 짧게 페이드아웃한다.
3. 침대 Sound Mix를 해제한다.
4. 현재 활성 FixedCameraZone이 있으면 그 카메라로, 없으면 Character 카메라로 복귀한다.
5. 저장했던 HUD와 월드 라벨 상태를 복원한다.
6. Character Movement와 일반 입력을 복원한다.
7. 상태를 `Idle`로 바꾼다.

`ABalhwajeomCameraCharacter`에는 외부 연출 종료 후 현재 `ActiveCameraZone` 또는 Character 중 올바른 탐색 카메라를 다시 활성화하는 `RestoreExplorationView()` 같은 작은 공개 함수를 추가하는 것이 안전하다. 침대 액터가 `ActiveCameraZone` 내부 상태를 직접 추측해서는 안 된다.

## 11. UI 정책

진입 전:

```text
[F] 앉기
```

진입 후에는 1.5~2초 동안만 다음 문구를 보여주고 자연스럽게 사라지게 한다.

```text
[F] 일어나기
```

일반 조사 라벨, 상호작용 프롬프트, 조사 메시지, 카메라 HUD, 태블릿 UI는 숨긴다. 단, 운영체제/엔진 자막과 접근성 표시는 숨기지 않는다.

현재 사진 카메라의 월드 라벨 숨김은 `ABalhwajeomEvidenceActor` 순회에 묶여 있다. 침대 기능을 추가할 때는 이를 공용 Presentation Suppression API로 올리는 것이 좋다. 최소 구현이라면 `UPlayerInteractionComponent`에 `SetPresentationSuppressed(bool)`와 변경 delegate를 두고 월드 라벨 Presenter들이 구독하게 한다.

## 12. 진입 거부 및 예외 처리

| 상황 | 처리 |
|---|---|
| 태블릿 열림/닫힘 애니메이션 중 | 진입하지 않음 |
| 사진 카메라 모드/전환 중 | 진입하지 않음 |
| 이미 Entering 또는 Exiting | 추가 입력 무시 |
| 유효 보이스 0개 | 앉기 허용, 환경음만 유지 |
| Sound 에셋 로드 실패 | 해당 항목 건너뛰고 다음 정적 예약 |
| Emitter 누락 | 기본 Voice Origin에서 재생 |
| Bed Actor 파괴/레벨 전환 | 타이머, 오디오, Mix, 입력 잠금 정리 |
| 플레이어 사망/Pawn 교체 | 세션 강제 종료 후 입력 잠금 해제 |
| 일시정지 | 게임 시간 기준 타이머도 함께 멈춤 |

침대가 진입 불가능할 때 별도 오류 토스트는 띄우지 않는다. 태블릿이나 카메라 모드에서는 침대 프롬프트 자체가 보이지 않게 하는 편이 자연스럽다.

## 13. Blueprint 배치 규칙

권장 에셋:

```text
Content/Balhwajeom/Gameplay/Interaction/BedMemory/
  BP_BedMemory
  DA_BedMemoryPlaylist_FamilyRoom

Content/Balhwajeom/Audio/Music/
  MX_BedMemory

Content/Balhwajeom/Audio/SFX/MemoryVoice/
  ATT_MemoryVoice
  (기존 보이스 에셋 참조)
```

`BP_BedMemory`에서 기획자가 조정할 값:

- 상호작용 거리와 `[F] 앉기` 라벨
- SeatedCamera 위치, 회전, FOV
- Voice Origin과 Desk/Door/Bed/Hall/Photo Anchor 위치
- Playlist Data Asset
- 카메라 블렌드 시간
- Sound Mix
- 일어나기 안내 표시 시간

레벨의 기존 침대 메시를 교체하기 어렵다면 보이지 않는 `BP_BedMemory`를 침대 위에 겹쳐 배치하고, Collision과 Anchor만 사용해도 된다.

## 14. 테스트 기준

### 자동화 가능한 단위 테스트

- 거리 밖에서는 침대 Interaction이 실행되지 않는다.
- 같은 셔플 백 안에서 VoiceID가 중복되지 않는다.
- 새 셔플 백의 첫 항목이 직전 항목과 같지 않다(항목 2개 이상).
- 잠금 조건을 만족하지 않은 항목은 백에 들어가지 않는다.
- 0개, 1개, 2개 음성 목록에서도 무한 루프나 배열 오류가 없다.
- 퇴장 시 예약된 타이머가 더 이상 보이스를 재생하지 않는다.
- 중복 진입/퇴장 호출이 Sound Mix와 입력 잠금을 중복 적용하거나 해제하지 않는다.

### PIE 수동 검증

1. 침대 근처에서만 `[F] 앉기`가 표시되는가.
2. 앉는 동안 캐릭터 이동, 점프, 카메라, 태블릿, 조사가 모두 막히는가.
3. 첫 음성 전에 1~2초의 정적이 존재하는가.
4. 음성이 서로 겹치지 않는가.
5. 같은 음성이 한 바퀴 안에서 반복되지 않는가.
6. Desk/Door/Bed/Hall 위치가 헤드폰으로 구분되면서도 대사가 잘 들리는가.
7. 정적 중 환경음과 BGM이 살아 있는가.
8. 자막을 켠 경우 HUD가 사라져도 자막은 보이는가.
9. 어느 타이밍에 일어나도 즉시 안전하게 복귀하는가.
10. FixedCameraZone 안에서 진입/퇴장해도 올바른 탐색 카메라로 돌아오는가.
11. 20회 이상 반복해도 입력 잠금, HUD 숨김, Sound Mix가 남지 않는가.

### 감성 플레이테스트 질문

- 보이스가 정보 목록이 아니라 방에 남은 생활 소리처럼 느껴지는가.
- 정적이 지루한가, 머무를 틈으로 느껴지는가.
- 공간 방향성이 몰입을 돕는가, 대사를 찾게 만들어 산만한가.
- 앉은 카메라가 웜톤 햇빛과 방의 흔적을 충분히 보여주는가.
- 일어나기 안내가 너무 오래 남아 장면을 설명하고 있지는 않은가.

## 15. 구현 순서

### 1단계 — 기능 골격

- `IWorldInteractable` 추가
- `UPlayerInteractionComponent`의 비텍스트 상호작용 경로 추가
- `ABedMemoryActor` 상태 머신과 카메라/입력 진입·복원 구현
- 빈 플레이리스트로 안전한 진입/퇴장 검증

### 2단계 — 오디오

- Playlist Data Asset과 Voice Entry 구현
- 셔플 백, 정적 타이머, `OnAudioFinished` 연결
- Emitter Anchor 공간 재생
- Sound Class, Attenuation, Sound Mix 연결

### 3단계 — 진행 상태와 UI

- 보이스 청취 Gameplay Tag 기록 경로 연결
- `DiscoveredOnly` 필터 연결
- 공용 월드 라벨/HUD 억제와 복원
- 일어나기 안내와 자막 검증

### 4단계 — 연출 조정

- SeatedCamera 구도와 블렌드 조정
- 웜톤 햇빛, BGM 덕킹, 정적 길이 플레이테스트
- 필요할 때만 착석 Montage/제한 Look을 2차 범위로 추가

## 16. 완료 조건

다음 조건을 모두 만족하면 1차 기능 완료로 본다.

- 플레이어가 침대에서 선택적으로 앉고 언제든 일어날 수 있다.
- 앉는 동안 조사 진행도는 전혀 바뀌지 않는다.
- 이미 해금된 보이스만 한 바퀴 무중복으로 재생된다.
- 음성 사이 정적과 공간 방향이 체감된다.
- 카메라, 입력, HUD, 오디오 믹스가 모든 퇴장 경로에서 정상 복원된다.
- 자막 접근성이 유지된다.
- 기능을 20회 반복해도 타이머, 오디오, 입력 잠금이 누적되지 않는다.

