# Tablet Internet Browser Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a persistent, offline, tabbed browser application to the existing in-game tablet.

**Architecture:** Keep deterministic browser session state and common behavior in runtime C++, while each authored page remains a Widget Blueprint derived from one shared page class. Reuse the existing Investigation subsystem for keyword acquisition and the existing editor builder for creating and embedding Widget Blueprint assets.

**Tech Stack:** Unreal Engine 5.7, C++20 through Unreal Build Tool, UMG, Slate pointer events, Unreal Automation Tests, Widget Blueprint editor generation.

**Spec:** `Docs/superpowers/specs/2026-09-09-tablet-internet-browser-design.md`

## Global Constraints

- Live web access is not implemented; every page is authored local content.
- Do not add address, back, forward, or refresh controls.
- Do not add browser state to SaveGame.
- Keep Main pinned and unique; keep every other page unique while open.
- Use `UBalhwajeomInvestigationSubsystem::AcquireWord` with `EWordAcquisitionSource::Browser` for keyword acquisition.
- Preserve unrelated modified HUD assets.

---

### Task 1: Browser session state

**Files:**
- Create: `Source/Balhwajeom/Public/Tablet/BalhwajeomInternetTypes.h`
- Create: `Source/Balhwajeom/Private/Tablet/BalhwajeomInternetTypes.cpp`
- Create: `Source/Balhwajeom/Private/Tablet/Test/BalhwajeomInternetSessionTest.cpp`

**Interfaces:**
- Produces: `EBalhwajeomInternetPage`, `FBalhwajeomInternetSessionState::Reset`, `OpenPage`, `ClosePage`, `IsPageOpen`, `SetScrollOffset`, `GetScrollOffset`, `SetMaximized`, and `SetNormalWindowPosition`.
- Consumes: no UMG widgets or engine world state.

- [ ] **Step 1: Write failing state tests**

```cpp
FBalhwajeomInternetSessionState State;
State.Reset();
TestEqual(TEXT("Main is the only default tab"), State.GetOpenPages().Num(), 1);
TestTrue(TEXT("News1 opens once"), State.OpenPage(EBalhwajeomInternetPage::News1));
TestFalse(TEXT("News1 does not duplicate"), State.OpenPage(EBalhwajeomInternetPage::News1));
TestFalse(TEXT("Main cannot close"), State.ClosePage(EBalhwajeomInternetPage::Main));
```

- [ ] **Step 2: Build and run the new test to verify RED**

Run the Editor build and `Automation RunTests Balhwajeom.Tablet.Internet.Session`. Expected: compilation fails because the new production types do not exist.

- [ ] **Step 3: Implement the minimal state type**

Implement literal page validation, ordered unique tabs, active-page fallback to the previous tab, non-negative scroll offsets, maximize state, and full-window clamping within `1440x1080`.

- [ ] **Step 4: Build and run the test to verify GREEN**

Expected: all `Balhwajeom.Tablet.Internet.Session` assertions pass.

### Task 2: Shared page, tab, and keyword widgets

**Files:**
- Create: `Source/Balhwajeom/Public/Tablet/BalhwajeomInternetPageWidget.h`
- Create: `Source/Balhwajeom/Private/Tablet/BalhwajeomInternetPageWidget.cpp`
- Create: `Source/Balhwajeom/Public/Tablet/BalhwajeomInternetTabWidget.h`
- Create: `Source/Balhwajeom/Private/Tablet/BalhwajeomInternetTabWidget.cpp`
- Create: `Source/Balhwajeom/Public/Tablet/BalhwajeomInternetKeywordWidget.h`
- Create: `Source/Balhwajeom/Private/Tablet/BalhwajeomInternetKeywordWidget.cpp`
- Extend test: `Source/Balhwajeom/Private/Tablet/Test/BalhwajeomInternetSessionTest.cpp`

**Interfaces:**
- Consumes: `EBalhwajeomInternetPage` and the existing Investigation subsystem.
- Produces: page-link, tab-select, tab-close delegates; `SetupPage`, `SetupTab`, `SetupKeyword`; saved page scroll offset and acquired-keyword presentation.

- [ ] **Step 1: Add failing tests for page metadata and tab closeability**

```cpp
TestEqual(TEXT("News title"), InternetPageTitle(EBalhwajeomInternetPage::News1).ToString(), FString(TEXT("뉴스 1")));
TestFalse(TEXT("Main tab is not closeable"), IsInternetPageCloseable(EBalhwajeomInternetPage::Main));
TestTrue(TEXT("News tab is closeable"), IsInternetPageCloseable(EBalhwajeomInternetPage::News1));
```

- [ ] **Step 2: Run the focused test to verify RED**

Expected: the title and closeability helpers are missing.

- [ ] **Step 3: Implement shared widgets and keyword flow**

Bind optional named link buttons on pages, bind select and close buttons on tabs, bind the keyword button to `AcquireWord`, log successful Browser acquisition, and switch acquired text to a muted acquired color.

- [ ] **Step 4: Run focused tests and build to verify GREEN**

Expected: session tests pass and all three widget classes compile.

### Task 3: Browser window controller

**Files:**
- Create: `Source/Balhwajeom/Public/Tablet/BalhwajeomInternetWidget.h`
- Create: `Source/Balhwajeom/Private/Tablet/BalhwajeomInternetWidget.cpp`
- Extend test: `Source/Balhwajeom/Private/Tablet/Test/BalhwajeomInternetSessionTest.cpp`

**Interfaces:**
- Consumes: session state, page widgets, tab widgets, `HB_TabBar`, `WS_PageContent`, `SizeBox_BrowserWindow`, `BRD_TitleBar`, `BTN_Maximize`, and `BTN_Close`.
- Produces: `OpenPage`, `ClosePage`, `ToggleMaximize`, `PrepareForDesktopOpen`, `OnCloseRequested`, and read-only state queries used by smoke tests.

- [ ] **Step 1: Add a failing lifecycle test**

Test that initial setup creates Main, opening News1 twice produces one News1 tab, closing an active News1 selects Main, and preparing after the X action restores normal mode without resetting open tabs or the normal position.

- [ ] **Step 2: Run the focused test to verify RED**

Expected: `UBalhwajeomInternetWidget` and its lifecycle API do not exist.

- [ ] **Step 3: Implement the controller and drag behavior**

Cache one widget instance per open page, rebuild the tab strip from ordered state, switch active page without recreating it, save scroll before switching, maximize to `1440x1080`, and update the normal position only while the left mouse button drags `BRD_TitleBar`.

- [ ] **Step 4: Run tests and build to verify GREEN**

Expected: session and controller tests pass; C++ build succeeds.

### Task 4: Generate the Internet Widget Blueprints

**Files:**
- Modify: `Source/BalhwajeomEditor/Private/Tablet/TabletWidgetBlueprintLibrary.cpp`
- Modify: `Source/BalhwajeomEditor/Public/Tablet/TabletWidgetBlueprintLibrary.h`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_Internet.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetTab.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetKeyword.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetPage_Main.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetPage_Weather.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetPage_News1.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetPage_News2.uasset`
- Generate: `Content/Balhwajeom/UI/Tablet/WBP_InternetPage_Ad.uasset`

**Interfaces:**
- Consumes: the four shared runtime widget classes and existing `FBuilder` helpers.
- Produces: compiled designer-editable Widget Blueprints with exact `BindWidgetOptional` names.

- [ ] **Step 1: Extend the smoke test with missing-asset assertions**

Require all eight Blueprint classes and verify that every page class derives from `UBalhwajeomInternetPageWidget`.

- [ ] **Step 2: Run the smoke test to verify RED**

Expected: the eight assets cannot be loaded.

- [ ] **Step 3: Add focused builder functions**

Build the browser chrome, fixed window, tab row, scrollable page roots, four Main cards, authored page text, image placeholders, and the `WORD_RELATED_AGENCY` keyword row. Preserve existing assets unless the explicit redesign entry point is called.

- [ ] **Step 4: Generate assets and run the smoke test to verify GREEN**

Expected: all eight assets load, compile, and expose the required widgets.

### Task 5: Embed Internet in the tablet and verify the full flow

**Files:**
- Modify: `Source/Balhwajeom/Public/Tablet/BalhwajeomTabletWidget.h`
- Modify: `Source/Balhwajeom/Private/Tablet/BalhwajeomTabletWidget.cpp`
- Modify: `Source/BalhwajeomEditor/Private/Tablet/TabletWidgetBlueprintLibrary.cpp`
- Regenerate: `Content/Balhwajeom/UI/Tablet/WBP_Tablet.uasset`
- Update: `Docs/Systems/Tablet/태블릿_브라우져.md`

**Interfaces:**
- Consumes: `WBP_Internet` and `FInternetCloseRequestedSignature`.
- Produces: Internet icon entry, browser close-to-Home behavior, and full tablet smoke coverage.

- [ ] **Step 1: Add failing tablet smoke assertions**

Require `GetInternetWidget()` to return the embedded child, click the Internet icon, open and focus News1, preserve tabs across tablet visibility changes, click Internet close, and confirm Home is active while the Internet session remains intact.

- [ ] **Step 2: Run smoke verification to confirm RED**

Expected: the tablet has no embedded Internet child or close delegate.

- [ ] **Step 3: Replace the Internet placeholder with `WBP_Internet`**

Bind `OnCloseRequested` in `UBalhwajeomTabletWidget`, call `PrepareForDesktopOpen` only when the desktop Internet icon is selected, and handle X by returning to Home without destroying either widget.

- [ ] **Step 4: Regenerate `WBP_Tablet` and run full verification**

Run the focused automation test, editor smoke tests, and a clean Editor build. Confirm no unrelated HUD assets changed during generation.

- [ ] **Step 5: Update the browser handoff document**

Record the class and asset ownership, exact edit locations for page content, runtime persistence boundary, keyword flow, and manual PIE checks for links, tabs, scroll, drag, maximize, close, T close or reopen, and new-game reset.
