# Balhwajeom

2학기 미니 프로젝트를 위한 Unreal Engine 협업 저장소입니다.

이 문서는 아트, 기획, 프로그래밍 팀원이 프로젝트 폴더를 같은 기준으로 사용하고 GitHub Desktop으로 안전하게 협업하기 위한 가이드입니다.

## 가장 중요한 규칙

1. 작업을 시작하기 전에 GitHub Desktop에서 **Fetch origin**과 **Pull origin**을 실행합니다.
2. 개인 테스트는 `Content/Developers/본인폴더` 또는 `Content/Balhwajeom/Maps/Test`에서 진행합니다.
3. `Developers`와 `Maps/Test`는 GitHub에 올라가지 않으므로 개인 테스트 파일의 백업 공간으로 간주하면 안 됩니다.
4. 개인 테스트가 완료되어 팀에서 사용해야 하는 에셋은 적절한 정식 폴더로 옮긴 뒤 커밋합니다.
5. 개인 테스트 폴더의 에셋을 정식 맵이나 정식 에셋에서 참조하지 않습니다.
6. `.uasset`과 `.umap`은 병합하기 어려우므로 같은 에셋을 여러 사람이 동시에 수정하지 않습니다.

## GitHub에 보이지 않는 폴더

다음 두 폴더는 `.gitignore`에 등록되어 있습니다.

```text
Content/Developers/
Content/Balhwajeom/Maps/Test/
```

따라서 GitHub 저장소를 처음 Clone한 사람에게는 이 폴더들이 보이지 않을 수 있습니다. 이는 정상입니다. Git은 빈 폴더를 저장하지 않으며, ignore된 폴더와 그 안의 파일도 업로드하지 않습니다.

필요한 팀원은 자신의 컴퓨터에서 직접 폴더를 만들어 사용합니다.

### Developers 폴더

개인 블루프린트, 임시 에셋 수정본, 기능 실험 등 개인 샌드박스 용도입니다.

```text
Content/
└─ Developers/
   ├─ RealJaemin/
   ├─ ArtistName/
   └─ PlannerName/
```

Unreal Content Browser에서 보이지 않으면 다음 옵션을 켭니다.

```text
Content Browser → Settings → Show Developers Content
```

본인의 Windows 사용자 이름을 기준으로 개발자 폴더가 표시될 수 있습니다. 반드시 본인 폴더 안에서만 개인 실험을 진행합니다.

### Maps/Test 폴더

GitHub에 올릴 필요가 없는 개인 테스트 레벨을 저장하는 공간입니다.

```text
Content/Balhwajeom/Maps/Test/
```

Clone 후 폴더가 없다면 Unreal Editor에서 해당 경로에 `Test` 폴더를 만든 후 테스트 맵을 저장합니다.

여러 팀원이 함께 사용해야 하는 테스트 맵은 이곳에 두지 않습니다. 공유가 필요해진 맵은 `Maps/Main` 또는 팀에서 합의한 추적 폴더로 옮깁니다.

> 주의: ignored 폴더의 에셋을 정식 에셋이 참조하면 다른 팀원의 프로젝트에는 해당 파일이 없기 때문에 참조가 깨집니다.

## 프로젝트 폴더 구조

```text
Content/
└─ Balhwajeom/
   ├─ Core/
   │  ├─ GameModes/
   │  ├─ Player/
   │  ├─ Camera/
   │  └─ Input/
   ├─ Gameplay/
   │  ├─ Interaction/
   │  ├─ Combat/
   │  └─ Items/
   ├─ Characters/
   │  ├─ Player/
   │  ├─ Enemies/
   │  └─ NPCs/
   ├─ Environment/
   │  ├─ Meshes/
   │  ├─ Materials/
   │  └─ Textures/
   ├─ UI/
   │  ├─ HUD/
   │  ├─ Menus/
   │  └─ Common/
   ├─ Audio/
   │  ├─ Music/
   │  └─ SFX/
   ├─ FX/
   │  ├─ Niagara/
   │  └─ Materials/
   ├─ Maps/
   │  ├─ Main/
   │  └─ Test/       # 로컬 전용, GitHub 업로드 제외
   ├─ Cinematics/
   │  └─ Sequences/
   └─ Data/
      ├─ DataAssets/
      └─ DataTables/
```

## 폴더별 용도

| 폴더 | 용도 | 주요 사용자 |
|---|---|---|
| `Core/GameModes` | 게임의 기본 규칙과 GameMode | 프로그래밍, 기획 |
| `Core/Player` | 플레이어 Character, Controller 및 공통 플레이어 로직 | 프로그래밍 |
| `Core/Camera` | 카메라 액터, 카메라 존 및 카메라 동작 | 프로그래밍, 기획 |
| `Core/Input` | Input Action과 Input Mapping Context | 프로그래밍 |
| `Gameplay/Interaction` | 상호작용 시스템과 상호작용 오브젝트 | 프로그래밍, 기획 |
| `Gameplay/Combat` | 전투, 공격, 피해 및 전투 관련 기능 | 프로그래밍, 기획 |
| `Gameplay/Items` | 아이템, 획득물 및 아이템 관련 데이터 | 프로그래밍, 기획 |
| `Characters/Player` | 플레이어 전용 모델, 애니메이션 및 관련 에셋 | 아트, 프로그래밍 |
| `Characters/Enemies` | 적 캐릭터 전용 에셋 | 아트, 기획, 프로그래밍 |
| `Characters/NPCs` | 비전투 NPC 전용 에셋 | 아트, 기획, 프로그래밍 |
| `Environment/Meshes` | 배경 및 프롭 Static Mesh | 아트 |
| `Environment/Materials` | 배경용 Material과 Material Instance | 아트 |
| `Environment/Textures` | 배경용 Texture | 아트 |
| `UI/HUD` | 인게임 HUD 위젯 | UI, 기획, 프로그래밍 |
| `UI/Menus` | 타이틀, 옵션, 일시정지 등 메뉴 | UI, 기획, 프로그래밍 |
| `UI/Common` | 여러 UI에서 공통으로 사용하는 위젯과 리소스 | UI, 프로그래밍 |
| `Audio/Music` | 배경 음악 | 사운드, 기획 |
| `Audio/SFX` | 효과음 | 사운드, 기획 |
| `FX/Niagara` | Niagara System, Emitter 및 관련 효과 | FX, 아트 |
| `FX/Materials` | 이펙트 전용 Material | FX, 아트 |
| `Maps/Main` | 팀이 공유하는 정식 플레이 레벨 | 전 직군 |
| `Maps/Test` | GitHub에 올리지 않는 로컬 테스트 레벨 | 전 직군 |
| `Cinematics/Sequences` | Level Sequence와 시네마틱 에셋 | 시네마틱, 기획, 아트 |
| `Data/DataAssets` | 설정값과 게임 데이터용 Data Asset | 기획, 프로그래밍 |
| `Data/DataTables` | CSV 기반 데이터와 Data Table | 기획, 프로그래밍 |

폴더가 필요 이상으로 세분화되지 않도록 현재 구조를 우선 사용합니다. 에셋 수가 충분히 많아졌을 때 팀과 합의하여 하위 폴더를 추가합니다.

## 직군별 작업 예시

### 아트

- 개인 메시나 머티리얼 실험은 `Content/Developers/본인폴더`에서 진행합니다.
- 확정된 배경 에셋은 `Environment`의 해당 폴더로 옮깁니다.
- 캐릭터 전용 에셋은 `Characters/Player`, `Characters/Enemies`, `Characters/NPCs`로 구분합니다.
- 테스트용 배치 레벨은 `Maps/Test`에 저장합니다.
- 정식 맵에 적용하기 전 담당자에게 같은 에셋을 수정 중인지 확인합니다.

Photoshop, Blender, FBX 같은 원본 작업 파일을 저장소에 직접 추가해야 한다면 먼저 팀에 알립니다. 현재 Git LFS는 Unreal 에셋만 대상으로 하므로 원본 파일 확장자에 대한 LFS 설정이 추가로 필요할 수 있습니다.

### 기획

- 수치와 설정 데이터는 `Data/DataAssets` 또는 `Data/DataTables`에 둡니다.
- 기능 검증용 블루프린트와 임시 데이터는 `Content/Developers/본인폴더`에서 제작합니다.
- 개인 테스트 레벨은 `Maps/Test`에서 작업합니다.
- 프로그래머나 아티스트에게 전달할 테스트 결과는 ignored 폴더에만 남기지 말고 문서, 이슈 또는 정식 폴더로 옮겨 공유합니다.

### 프로그래밍

- C++ 코드는 `Source/Balhwajeom` 아래에서 관리하며 항상 Git에 포함합니다.
- 공통 기반 블루프린트는 `Core`, 실제 게임 기능은 `Gameplay`에 둡니다.
- 임시 블루프린트 실험은 `Content/Developers/본인폴더`에서 진행합니다.
- C++ 테스트 코드는 ignore 폴더에 두지 않고 프로젝트의 정식 테스트 경로에 추가합니다.
- 임시 코드 작업은 ignore 폴더 대신 별도의 Git 브랜치를 사용합니다.

## 개인 테스트를 정식 에셋으로 전환하는 방법

1. 테스트가 완료되었는지 확인합니다.
2. 에셋 이름과 참조 관계를 정리합니다.
3. Unreal Content Browser에서 에셋을 적절한 정식 폴더로 이동합니다.
4. Windows 탐색기로 `.uasset`이나 `.umap`을 직접 이동하지 않습니다.
5. 이동 후 관련 폴더에서 **Fix Up Redirectors in Folder**를 실행합니다.
6. 정식 맵이나 다른 에셋에서 참조가 정상인지 확인합니다.
7. GitHub Desktop에서 이동된 파일과 함께 필요한 의존 에셋이 모두 표시되는지 확인합니다.
8. 의미가 분명한 메시지로 커밋하고 Push합니다.

## 에셋 명명 규칙

| 에셋 종류 | 접두사 | 예시 |
|---|---|---|
| Blueprint | `BP_` | `BP_PlayerCharacter` |
| Widget Blueprint | `WBP_` | `WBP_MainMenu` |
| Animation Blueprint | `ABP_` | `ABP_Player` |
| Input Action | `IA_` | `IA_Move` |
| Input Mapping Context | `IMC_` | `IMC_Player` |
| Data Asset | `DA_` | `DA_ItemSword` |
| Data Table | `DT_` | `DT_Items` |
| Material | `M_` | `M_MasterSurface` |
| Material Instance | `MI_` | `MI_Wood` |
| Texture | `T_` | `T_Wood_BaseColor` |
| Niagara System | `NS_` | `NS_HitImpact` |
| Sound Cue | `SC_` | `SC_Footstep` |
| Level | `L_` | `L_Main` |

기존 에셋과 충돌하지 않는 범위에서 위 규칙을 적용합니다. 폴더명과 에셋명에는 가급적 영문과 숫자를 사용합니다.

## GitHub Desktop 작업 순서

### 작업 시작 전

1. 가능하면 Unreal Editor를 닫습니다.
2. GitHub Desktop에서 현재 저장소를 선택합니다.
3. **Fetch origin**을 누릅니다.
4. 내려받을 변경 사항이 있으면 **Pull origin**을 누릅니다.
5. 다른 팀원이 같은 맵이나 블루프린트를 편집 중인지 확인합니다.

### 작업 완료 후

1. Unreal Editor에서 **Save All**을 실행합니다.
2. 에셋을 이동하거나 이름을 변경했다면 Redirector를 정리합니다.
3. GitHub Desktop의 Changes 목록을 확인합니다.
4. `Saved`, `Intermediate`, `Binaries`, `DerivedDataCache` 등이 보이면 커밋하지 말고 `.gitignore` 상태를 확인합니다.
5. 변경 내용을 설명하는 커밋 메시지를 작성합니다.
6. **Commit to 현재 브랜치**를 누릅니다.
7. **Push origin**을 누릅니다.

커밋 메시지는 무엇을 변경했는지 알아볼 수 있게 작성합니다.

```text
Add player interaction prototype
Update enemy balance data
Create main level lighting pass
Fix pause menu navigation
```

## Git LFS

Unreal Engine 바이너리 에셋은 Git LFS로 관리됩니다.

```text
*.uasset
*.umap
```

팀원은 `.gitattributes` 파일을 삭제하거나 수정하지 않습니다. GitHub Desktop으로 Clone과 Pull을 수행하면 LFS 파일도 함께 내려받습니다.

PSD, Blender, FBX, 고용량 음원과 영상 같은 원본 파일을 저장소에 추가하려면 먼저 프로그래머 또는 저장소 관리자와 상의합니다. 해당 확장자를 LFS에 등록하기 전에 커밋하면 일반 Git 기록에 대용량 파일이 남을 수 있습니다.

## Git에서 자동으로 제외되는 항목

다음 항목은 로컬 생성 파일이므로 GitHub에 업로드하지 않습니다.

```text
.vs/
Binaries/
Saved/
Intermediate/
DerivedDataCache/
Content/Developers/
Content/Balhwajeom/Maps/Test/
```

GitHub Desktop의 Changes 목록에 ignored 폴더의 파일이 나타나지 않는 것은 정상입니다.

## 문제 발생 시 확인 사항

- 다른 팀원의 에셋이 보이지 않으면 먼저 Fetch와 Pull을 했는지 확인합니다.
- 테스트 폴더가 없으면 로컬에서 직접 생성합니다.
- 테스트 에셋이 다른 PC에서 깨지면 정식 에셋이 ignored 폴더를 참조하고 있지 않은지 확인합니다.
- 동일한 `.uasset` 또는 `.umap` 충돌이 발생하면 임의로 덮어쓰지 말고 해당 파일의 작업자와 먼저 상의합니다.
- GitHub Desktop에 수백 개의 빌드 파일이 나타나면 커밋하지 말고 저장소 관리자에게 알립니다.
- 중요한 테스트 결과는 ignored 폴더에만 보관하지 말고 정식 폴더나 별도 백업 위치로 옮깁니다.
