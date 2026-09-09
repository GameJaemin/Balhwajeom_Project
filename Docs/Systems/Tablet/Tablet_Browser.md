# 태블릿 담당 작업 문서 — 브라우저 (Internet)

> 기준 문서: [Project-Progress-and-Roadmap.md](../../Project-Progress-and-Roadmap.md) 5장 백로그
> 담당 범위: `UBalhwajeomTabletWidget`의 `Internet` 페이지
> 같은 태블릿 폴더의 다른 문서: [Tablet_Folder.md](./Tablet_Folder.md), [Tablet_messenger.md](./Tablet_messenger.md)

핵심 파일: [BalhwajeomTabletWidget.h](../../../Source/Balhwajeom/Public/Tablet/BalhwajeomTabletWidget.h) / `.cpp`, `WBP_Tablet.uasset`(`Content/Balhwajeom/UI/Tablet`)

`UBalhwajeomTabletWidget`은 태블릿의 5개 페이지(`ETabletPage`: `Home`, `PersonFolder`, `Messenger`, `Internet`, `Memo`)를 한 위젯에서 전환하는 공용 클래스다. 이 헤더는 [Tablet_Folder.md](./Tablet_Folder.md), [Tablet_messenger.md](./Tablet_messenger.md) 담당자와 공유되므로, `UBalhwajeomTabletWidget.h`를 직접 수정할 때는 다른 두 담당자에게 미리 공지한다.

## 1. 현재 상태

**미구현 스텁**. `ETabletPage::Internet`, `BTN_Internet`/`HandleInternetClicked`, `BTN_InternetBack`은 존재하지만 실제 콘텐츠(웹페이지 형태의 UI, 검색/글 목록 등)는 아직 없다. 이번 일정에서 새로 설계·구현해야 하는 항목이다.

## 2. 구현 필요 사항

- [ ] 브라우저 콘텐츠의 최소 범위를 먼저 확정한다(예: 고정된 몇 개 게시글/검색 결과 열람, 키워드 획득 연동 여부 등). 범위가 크면 이번 마감(9/18) 내 완성 가능한 최소 버전으로 축소.
- [ ] 브라우저에서 키워드를 지급하거나 스토리에 영향을 준다면, 메신저와 동일하게 `InvestigationSubsystem::AcquireWord`를 통해서만 지급하도록 연결한다(별도 상태를 만들지 않는다).
- [ ] 콘텐츠 데이터가 필요하면 새 DataTable을 늘리기보다 기존 8종(`DT_Words`, `DT_Photos` 등) 재사용 가능 여부를 먼저 검토하고, 정말 필요할 때만 [DataTable-Handoff.md](../DataTable-Handoff.md) 담당자와 협의해 신규 테이블을 추가한다.
- [ ] `HandleInternetClicked`/`SetTabletPage(ETabletPage::Internet)` 흐름에 실제 콘텐츠 위젯을 연결하고 뒤로가기(`BTN_InternetBack`)가 `PageHistory`와 맞물려 정상 동작하는지 확인.

## 3. 작업 체크리스트 (일정 연동)

- **1단계(9/10~9/12, M1)**: 브라우저 최소 범위 확정 및 골격 구현 착수
- **2단계(9/13~9/15, M2)**: 브라우저 콘텐츠 구현 완료, 아트/이펙트/사운드 트랙과 UI 연출·효과음 연결
- **3단계(9/16~9/17, M3)**: 태블릿 내 다른 페이지(폴더/메신저)와 전환 회귀 테스트
- **4단계(9/18, M4)**: 최종 QA 대응

## 4. 완료 조건

- 브라우저 페이지가 확정된 최소 범위 기준으로 동작하며 프로토타입 전용 하드코딩이 없다.
- `Internet` 페이지 진입/이탈(`SetTabletPage`/`PageHistory`)이 다른 4개 페이지 전환과 충돌하지 않는다.

## 5. 주의 사항

- `WBP_Tablet`은 폴더·메신저 담당자와 공유되는 UI이므로 동시 편집 전 확인한다.
- 브라우저가 조사 상태를 다루게 되면 자체적으로 저장하지 않고 항상 [Subsystem-Handoff.md](../Subsystem-Handoff.md)의 InvestigationSubsystem을 통해서만 조회/갱신한다.
