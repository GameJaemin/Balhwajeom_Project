#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomInternetTypes.generated.h"

UENUM(BlueprintType)
enum class EBalhwajeomInternetPage : uint8
{
	Main,
	Weather,
	News1,
	News2,
	Ad
};

BALHWAJEOM_API bool IsValidInternetPage(EBalhwajeomInternetPage Page);
BALHWAJEOM_API FText InternetPageTitle(EBalhwajeomInternetPage Page);
BALHWAJEOM_API bool IsInternetPageCloseable(EBalhwajeomInternetPage Page);

/** Play-session-only browser state. This is intentionally not serialized to SaveGame. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FBalhwajeomInternetSessionState
{
	GENERATED_BODY()

public:
	static constexpr float TabletWidth = 1274.0f;
	static constexpr float TabletHeight = 907.0f;
	static constexpr float NormalWindowWidth = TabletWidth;
	static constexpr float NormalWindowHeight = TabletHeight;

	void Reset();
	bool OpenPage(EBalhwajeomInternetPage Page);
	bool ActivatePage(EBalhwajeomInternetPage Page);
	bool ClosePage(EBalhwajeomInternetPage Page);
	bool IsPageOpen(EBalhwajeomInternetPage Page) const;

	const TArray<EBalhwajeomInternetPage>& GetOpenPages() const { return OpenPages; }
	EBalhwajeomInternetPage GetActivePage() const { return ActivePage; }

	void SetScrollOffset(EBalhwajeomInternetPage Page, float Offset);
	float GetScrollOffset(EBalhwajeomInternetPage Page) const;

	void SetMaximized(bool bInMaximized) { bMaximized = bInMaximized; }
	bool IsMaximized() const { return bMaximized; }

	void SetNormalWindowPosition(FVector2D Position);
	FVector2D GetNormalWindowPosition() const { return NormalWindowPosition; }

private:
	UPROPERTY(Transient)
	TArray<EBalhwajeomInternetPage> OpenPages;

	UPROPERTY(Transient)
	TMap<EBalhwajeomInternetPage, float> ScrollOffsets;

	UPROPERTY(Transient)
	EBalhwajeomInternetPage ActivePage = EBalhwajeomInternetPage::Main;

	UPROPERTY(Transient)
	FVector2D NormalWindowPosition = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bMaximized = false;
};
