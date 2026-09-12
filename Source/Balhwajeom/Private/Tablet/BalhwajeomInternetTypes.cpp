#include "Tablet/BalhwajeomInternetTypes.h"

bool IsValidInternetPage(const EBalhwajeomInternetPage Page)
{
	return Page >= EBalhwajeomInternetPage::Main && Page <= EBalhwajeomInternetPage::Ad;
}

FText InternetPageTitle(const EBalhwajeomInternetPage Page)
{
	switch (Page)
	{
	case EBalhwajeomInternetPage::Weather:
		return NSLOCTEXT("TabletInternet", "WeatherTitle", "일기예보");
	case EBalhwajeomInternetPage::News1:
		return NSLOCTEXT("TabletInternet", "News1Title", "뉴스 1");
	case EBalhwajeomInternetPage::News2:
		return NSLOCTEXT("TabletInternet", "News2Title", "뉴스 2");
	case EBalhwajeomInternetPage::Ad:
		return NSLOCTEXT("TabletInternet", "AdTitle", "광고");
	case EBalhwajeomInternetPage::Main:
	default:
		return NSLOCTEXT("TabletInternet", "MainTitle", "메인");
	}
}

bool IsInternetPageCloseable(const EBalhwajeomInternetPage Page)
{
	return IsValidInternetPage(Page) && Page != EBalhwajeomInternetPage::Main;
}

void FBalhwajeomInternetSessionState::Reset()
{
	OpenPages.Reset();
	OpenPages.Add(EBalhwajeomInternetPage::Main);
	ScrollOffsets.Reset();
	ActivePage = EBalhwajeomInternetPage::Main;
	NormalWindowPosition = FVector2D::ZeroVector;
	bMaximized = false;
}

bool FBalhwajeomInternetSessionState::OpenPage(const EBalhwajeomInternetPage Page)
{
	if (!IsValidInternetPage(Page))
	{
		return false;
	}

	const bool bAdded = !OpenPages.Contains(Page);
	if (bAdded)
	{
		OpenPages.Add(Page);
	}
	ActivePage = Page;
	return bAdded;
}

bool FBalhwajeomInternetSessionState::ActivatePage(const EBalhwajeomInternetPage Page)
{
	if (!IsPageOpen(Page))
	{
		return false;
	}
	ActivePage = Page;
	return true;
}

bool FBalhwajeomInternetSessionState::ClosePage(const EBalhwajeomInternetPage Page)
{
	if (!IsInternetPageCloseable(Page))
	{
		return false;
	}

	const int32 ClosedIndex = OpenPages.IndexOfByKey(Page);
	if (ClosedIndex == INDEX_NONE)
	{
		return false;
	}

	const bool bClosedActivePage = ActivePage == Page;
	OpenPages.RemoveAt(ClosedIndex);
	ScrollOffsets.Remove(Page);
	if (bClosedActivePage)
	{
		const int32 FallbackIndex = FMath::Clamp(ClosedIndex - 1, 0, OpenPages.Num() - 1);
		ActivePage = OpenPages[FallbackIndex];
	}
	return true;
}

bool FBalhwajeomInternetSessionState::IsPageOpen(const EBalhwajeomInternetPage Page) const
{
	return OpenPages.Contains(Page);
}

void FBalhwajeomInternetSessionState::SetScrollOffset(
	const EBalhwajeomInternetPage Page,
	const float Offset)
{
	if (IsValidInternetPage(Page))
	{
		ScrollOffsets.FindOrAdd(Page) = FMath::Max(0.0f, Offset);
	}
}

float FBalhwajeomInternetSessionState::GetScrollOffset(const EBalhwajeomInternetPage Page) const
{
	if (const float* Offset = ScrollOffsets.Find(Page))
	{
		return *Offset;
	}
	return 0.0f;
}

void FBalhwajeomInternetSessionState::SetNormalWindowPosition(FVector2D Position)
{
	Position.X = FMath::Clamp(Position.X, 0.0f, TabletWidth - NormalWindowWidth);
	Position.Y = FMath::Clamp(Position.Y, 0.0f, TabletHeight - NormalWindowHeight);
	NormalWindowPosition = Position;
}
