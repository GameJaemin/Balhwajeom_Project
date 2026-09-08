# 조사 시스템 최종 프로토타입 레벨 테스트 가이드

## 테스트 레벨

- 에디터 경로: `/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype`
- 파일: `Content/Balhwajeom/Maps/Prototype/L_InvestigationPrototype.umap`
- GameMode: `BP_OrbitViewGameMode`
- Default Pawn: `BP_OrbitViewCharacter`

## 조작

- 이동: `WASD`
- 시점: 마우스
- 조사/상호작용: `F`
- 카메라 들기: 마우스 오른쪽 버튼
- 촬영: 카메라를 든 상태에서 마우스 왼쪽 버튼
- 태블릿 열기/닫기: `Tab`
- UI 닫기: `Esc`

## 권장 테스트 순서

1. 플레이를 시작하고 정면의 `돼지가 보던 거울` 오브젝트로 이동한다.
2. 오브젝트를 바라보고 `F`를 눌러 문서를 조사한다. 단일 선택지인 `거울` 키워드가 획득되어야 한다.
3. 마우스 오른쪽 버튼으로 카메라를 들고, 거울 오브젝트를 화면 중앙에 선명하게 맞춘 뒤 마우스 왼쪽 버튼으로 촬영한다.
4. 같은 방식으로 `스노우글로브`도 촬영한다.
5. `Tab`으로 태블릿을 열고 `여동생` 폴더에 거울 사진과 스노우글로브 사진이 생성됐는지 확인한다.
6. 거울 사진을 열고 문장 슬롯에 `돼지`, `거울` 순서로 키워드를 클릭해 배치한다.
7. 문장이 `돼지가 보던 거울`로 완성되고 사진이 완성 상태가 되는지 확인한다.
8. 진술 문서를 열어 `거울` 키워드를 선택한 뒤, 완성된 거울 사진을 증거로 선택하고 `자백 반증`을 실행한다.

## 기대 결과

- 촬영 가능 범위와 화면 중앙 판정이 맞을 때만 촬영된다.
- 같은 증거를 다시 촬영해도 사진이 중복 생성되지 않는다.
- 획득하지 않은 키워드는 문장 조립에 사용할 수 없다.
- 잘못된 키워드 순서는 사진을 완성시키지 않는다.
- 올바른 문장을 완성해도 화면이 자동으로 닫히지 않는다.
- 완성된 사진만 진술 반증 증거로 사용할 수 있다.

## 자동 검증

- 레벨 구조 검사: `Content/Python/validate_investigation_prototype_level.py`
- 레벨 재생성: `Content/Python/create_investigation_prototype_level.py`
- Orbit Pawn 중앙 초점 검사: `Balhwajeom.Camera.PrototypeCenteredFocus`
- 조사 시스템 자동화: `Balhwajeom.Investigation` 테스트 10개
