# Investigation prototype data

이 폴더의 CSV는 `Content/Balhwajeom/Data/Investigation` DataTable 8종(DT_Characters, DT_EvidenceDefinitions, DT_EvidenceStates, DT_KeywordChoices, DT_KeywordDocuments, DT_Photos, DT_Sentences, DT_Words)의 재임포트 원본이다.

- Row Name과 내부 ID는 항상 동일하게 유지한다.
- `DT_Photos.GrantedWordIDs`는 단순 `FName` 배열이므로 `(WORD_A,WORD_B)` 형식을 쓴다.
- `DT_Photos.WorldStoryCues`는 `(Text,StartTimeSeconds)` 구조체 배열이다. 첫 Cue는 0초부터 시작하고 이후 시간은 오름차순으로 입력한다. 한 Cue 안의 여러 줄은 `Text`에 개행으로 입력한다. `WorldStoryLines`는 기존 에셋 이관용 호환 필드이므로 신규 행에서는 비워 둔다.
- 문장 슬롯은 구조체 배열이므로 `((SlotIndex=0,CorrectWordID=WORD_A),...)` 형식을 쓴다.
- `PhotoSentenceID`가 비어 있으면 스토리 사진이다.
- `PreferredFocusDistance`, `FocusDistanceTolerance`, `bScaleFocusDistanceWithZoom`은 기본값을 사용할 때 CSV 헤더에서 생략한다.
