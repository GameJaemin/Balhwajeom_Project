# Investigation prototype data

이 폴더의 CSV는 `Content/Balhwajeom/Data/Investigation` DataTable 8종(DT_Characters, DT_EvidenceDefinitions, DT_EvidenceStates, DT_KeywordChoices, DT_KeywordDocuments, DT_Photos, DT_Sentences, DT_Words)의 재임포트 원본이다.

- Row Name과 내부 ID는 항상 동일하게 유지한다.
- `DT_Photos.GrantedWordIDs`는 단순 `FName` 배열이므로 `(WORD_A,WORD_B)` 형식을 쓴다.
- 헤더의 컬럼명은 구조체 필드명과 **정확히** 일치해야 한다. 뒤에 공백이 하나만 붙어도 UE가 그 컬럼을 조용히 무시하고 해당 필드는 비어 있는 채로 임포트된다.
- 구조체 배열 안의 문자열은 CSV 이스케이프 기준으로 `""`를 쓴다. `""""`로 쓰면 값이 `Text=""문구""`가 되어 파싱에 실패한다.
- `DT_Photos.WorldStoryCues`는 `(Text,StartTimeSeconds)` 구조체 배열이다. 첫 Cue는 0초부터 시작하고 이후 시간은 오름차순으로 입력한다. 한 Cue 안의 여러 줄은 `Text`에 개행으로 입력한다. `WorldStoryLines`는 기존 에셋 이관용 호환 필드이므로 신규 행에서는 비워 둔다.
- 문장 슬롯은 구조체 배열이므로 `((SlotIndex=0,CorrectWordID=WORD_A),...)` 형식을 쓴다.
- `PhotoSentenceID`가 비어 있으면 스토리 사진이다.
- `PreferredFocusDistance`, `FocusDistanceTolerance`, `bScaleFocusDistanceWithZoom`은 기본값을 사용할 때 CSV 헤더에서 생략한다.
- `CaptureBlockedLabel`은 현재 상태가 촬영 불가일 때 카메라 보조 라벨에 표시할 행동 안내다. 비우면 공통 촬영 불필요 문구가 표시된다.
