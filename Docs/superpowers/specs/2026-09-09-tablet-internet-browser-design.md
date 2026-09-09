# Tablet Internet Browser Design

## Goal

Build an offline, browser-shaped application inside `WBP_Tablet`. It displays authored Widget Blueprint pages rather than live internet content.

## Confirmed behavior

- The tablet Internet icon opens an embedded `WBP_Internet` page through the existing `ETabletPage::Internet` route.
- Pages are `Main`, `Weather`, `News1`, `News2`, and `Ad`.
- Each page is a separate Widget Blueprint whose parent class is `UBalhwajeomInternetPageWidget`.
- Main-page image and text cards open the matching page as a browser tab.
- A page can have only one open tab. Opening an already-open page focuses that tab.
- The Main tab always exists and cannot be closed.
- Other tabs can be selected and closed.
- Every page scrolls independently.
- Open tabs, active tab, page instances, scroll offsets, and the dragged normal-window position remain while the current play session is alive.
- No state is written to SaveGame. A new game creates a fresh browser session.
- The top-right controls are maximize or restore and close. There is no address bar, back, forward, or refresh.
- Maximized means the full logical tablet screen. A maximized window cannot be dragged.
- Closing Internet returns to the tablet desktop, leaves `WBP_Tablet` alive, preserves tabs and scroll positions, restores normal-window mode, and retains the last normal-window position.
- Normal-window size is fixed. The title bar is draggable, and the whole browser window is clamped inside the logical tablet screen.
- A highlighted word can grant an Investigation keyword through `UBalhwajeomInvestigationSubsystem::AcquireWord` with `EWordAcquisitionSource::Browser`.
- Keyword acquisition produces an Output Log entry only. An acquired keyword changes color and cannot grant itself twice.

## Architecture

### Runtime C++

- `FBalhwajeomInternetSessionState` owns open-page order, active page, saved scroll offsets, normal-window position, and maximize state. It contains no UMG code and is covered by automation tests.
- `UBalhwajeomInternetWidget` owns the browser window, tab widgets, cached page widgets, page switching, duplicate prevention, close and maximize actions, and title-bar dragging.
- `UBalhwajeomInternetPageWidget` is the common parent of all five page Widget Blueprints. It exposes page-link requests and tracks its `SB_PageContent` scroll offset.
- `UBalhwajeomInternetTabWidget` represents one selectable tab and hides its close action for Main.
- `UBalhwajeomInternetKeywordWidget` grants a word through the existing Investigation subsystem and refreshes its visual state when any word is acquired.
- `UBalhwajeomTabletWidget` embeds `WBP_Internet` exactly as it embeds `WBP_Messenger`, listens for the Internet close request, and returns to Home.

### Widget Blueprints

- `WBP_Internet` derives from `UBalhwajeomInternetWidget`.
- `WBP_InternetTab` derives from `UBalhwajeomInternetTabWidget`.
- `WBP_InternetKeyword` derives from `UBalhwajeomInternetKeywordWidget`.
- `WBP_InternetPage_Main`, `WBP_InternetPage_Weather`, `WBP_InternetPage_News1`, `WBP_InternetPage_News2`, and `WBP_InternetPage_Ad` derive from `UBalhwajeomInternetPageWidget`.
- The existing editor builder creates or refreshes these assets and embeds `WBP_Internet` as the fourth `WidgetSwitcher_TabletPage` child.

## Page content

- Main shows four large cards for Weather, News1, News2, and Ad. Each card includes an image area and a text label.
- Weather, News1, News2, and Ad contain authored image areas and enough text to demonstrate scrolling.
- News1 contains the emphasized word `관계기관`, mapped to `WORD_RELATED_AGENCY`.
- `DT_Words` remains authoritative. The editor setup adds `WORD_RELATED_AGENCY` only when it is missing; no browser-specific DataTable or SaveGame is introduced.

## Failure handling

- Missing or invalid page classes log an error and do not create a tab.
- Invalid page IDs and attempts to close Main are rejected without changing active state.
- Missing keyword definitions log an error and do not create runtime word state.
- Reinitialization does not duplicate tabs, page widgets, button bindings, or subsystem delegates.

## Verification

- Runtime automation tests cover default Main state, unique tabs, focus behavior, Main protection, active-tab fallback, scroll offsets, maximize state, and window-position clamping.
- Editor smoke tests instantiate the generated Widget Blueprints and exercise page opening, duplicate prevention, tab closing, maximize or restore, browser close, and tablet re-entry.
- A full `BalhwajeomEditor Win64 Development` build must succeed before completion is reported.
