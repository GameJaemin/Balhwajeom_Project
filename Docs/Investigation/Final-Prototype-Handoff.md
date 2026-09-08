# 조사 시스템 최종 프로토타입

## 구현 범위

- 사진 촬영 성공 시 `DT_Photos.GrantedWordIDs`의 키워드를 한 번만 지급한다.
- 키워드 문서와 선택지를 `DT_KeywordDocuments` / `DT_KeywordChoices`로 분리했다.
- 사진 분석은 모든 정답 슬롯이 채워지면 자동 판정한다.
- 진술서는 키워드와 완성 사진을 배치한 뒤 `자백 반증`을 눌러 판정한다.
- 미완성 추리 사진과 스토리 사진은 진술서 증거 후보에서 제외한다.
- 태블릿 인물 폴더는 `CharacterID`, `FolderName`, 촬영 여부, 문장 완료 여부를 읽어 파일과 `?`/`✓` 상태를 표시한다.
- 사진/진술서 팝업에서 획득 키워드와 완성 사진을 클릭해 슬롯에 순서대로 배치할 수 있다.

## 포함된 샘플 루프

`CHAPTER_01 / SISTER`용으로 돼지 장식과 거울(추리 사진), 스노우글로브(스토리 사진), F 조사 키워드 문서, 사진 분석 문장, 사진 1장을 요구하는 진술서를 제공한다. 기존 테이블이 비어 있을 때만 시드하며 기존 행은 덮어쓰지 않는다.

CSV 재임포트 예시는 `Scripts/Investigation`에 있다.

## 검증

- `BalhwajeomEditor Win64 Development` 빌드 성공
- `Balhwajeom.Investigation` 자동화 테스트 10개 성공
- 실제 설정 DataTable 전체 교차참조 검증 성공
- `WBP_Tablet` 재생성 및 태블릿/메신저 스모크 테스트 성공

## 에디터 유틸리티

- `Content/Python/upgrade_investigation_data.py`: 기존 중첩 선택지 마이그레이션, 빈 테이블 샘플 시드
- `Content/Python/redesign_tablet_widget_blueprint.py`: 최종 태블릿 WBP 재생성
- `Content/Python/smoke_test_tablet_widget.py`: 태블릿 구조와 탐색 검증
