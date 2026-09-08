# Investigation prototype data

이 폴더의 CSV는 `Content/Balhwajeom/Data/Investigation` DataTable 7종의 재임포트 원본 예시다.

- Row Name과 내부 ID는 항상 동일하게 유지한다.
- `DT_Photos.GrantedWordIDs`는 단순 `FName` 배열이므로 `(WORD_A,WORD_B)` 형식을 쓴다.
- 문장 슬롯은 구조체 배열이므로 `((SlotIndex=0,CorrectWordID=WORD_A),...)` 형식을 쓴다.
- `PhotoSentenceID`가 비어 있으면 스토리 사진이다.
- `PreferredFocusDistance`, `FocusDistanceTolerance`, `bScaleFocusDistanceWithZoom`은 기본값을 사용할 때 CSV 헤더에서 생략한다.
