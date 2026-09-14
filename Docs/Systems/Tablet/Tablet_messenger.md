# 태블릿 메신저 UI

> 기준일: 2026-09-14
> 현재 범위: 실제 대화 데이터가 확정되기 전까지 사용하는 정적 UI

## 현재 동작

`WBP_Messenger`는 `/Game/Balhwajeom/UI/Tablet/Messenger`의 최종 이미지를 사용한다.
Data Asset이나 조사 Subsystem에서 대화를 읽지 않으며 다음 기능만 제공한다.

- 아버지, 어머니, 막내, 형 채팅방 표시
- 채팅방 클릭 시 선택 배경 이동
- 선택한 채팅방 이름 표시
- 빈 대화 영역 유지
- 우측 상단 닫기 버튼으로 태블릿 이전 화면 복귀

`UBalhwajeomMessengerWidget`은 선택된 방 ID와 닫기 이벤트만 관리한다. 기존
안 읽음 API는 `WBP_Tablet` 호환을 위해 항상 0을 반환하는 형태로 임시 유지한다.

## 핵심 파일

- `Source/Balhwajeom/Public/Tablet/BalhwajeomMessengerWidget.h`
- `Source/Balhwajeom/Private/Tablet/BalhwajeomMessengerWidget.cpp`
- `Content/Balhwajeom/UI/Tablet/WBP_Messenger.uasset`
- `Scripts/Tablet/RedesignMessengerWidget.py`

## 제거 가능한 레거시

새 정적 UI 검증 후 아래 항목은 메신저에서 더 이상 사용하지 않으므로 제거할 수 있다.

- `/Game/Balhwajeom/Data/Messenger/DA_MessengerCatalog`
- `/Game/Balhwajeom/Data/Messenger/Rooms/DA_MessengerRoom_*`
- `WBP_MessengerRoom`
- `WBP_MessengerMessage`
- `WBP_MessengerKeyword`
- `WBP_MessengerDateSeparator`
- `BalhwajeomMessengerDataAssets.h/.cpp`
- `BalhwajeomMessengerTypes.h`
- `BalhwajeomMessengerRoomWidget.h/.cpp`
- `BalhwajeomMessengerMessageWidget.h/.cpp`
- `BalhwajeomMessengerKeywordWidget.h/.cpp`
- `BalhwajeomMessengerDateSeparator.h/.cpp`
- 에디터 라이브러리의 `CreateMessengerDataAssets`, 타임라인 생성·검증 코드
- 기존 Data Asset 기반 메신저 테스트

실제 대화 데이터 방식이 확정되기 전까지는 레거시 에셋을 즉시 삭제하지 않는다. 삭제할
때는 Unreal Content Browser의 Reference Viewer로 참조가 0건인지 확인하고 삭제한다.

## 검증

자동화 테스트:

```text
Balhwajeom.Tablet.Messenger.StaticRoomSelection
```

이 테스트는 WBP 구성 요소, 650x699 디자인 크기, 네 방 선택, 빈 대화 상태와 잘못된
Room ID 거부를 확인한다.
