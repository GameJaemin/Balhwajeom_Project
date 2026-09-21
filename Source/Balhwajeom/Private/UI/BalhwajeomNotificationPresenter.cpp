#include "UI/BalhwajeomNotificationPresenter.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/BalhwajeomNotificationBannerWidget.h"


UBalhwajeomNotificationPresenter::UBalhwajeomNotificationPresenter()
{
	PrimaryComponentTick.bCanEverTick = false;
	BannerWidgetClass = UBalhwajeomNotificationBannerWidget::StaticClass();
}

void UBalhwajeomNotificationPresenter::ShowNotification(const FText& Message)
{
	if (Message.IsEmpty())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (!BannerWidget)
	{
		TSubclassOf<UBalhwajeomNotificationBannerWidget> WidgetClass = BannerWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UBalhwajeomNotificationBannerWidget::StaticClass();
		}
		BannerWidget = CreateWidget<UBalhwajeomNotificationBannerWidget>(PlayerController, WidgetClass);
		if (!BannerWidget)
		{
			return;
		}
	}

	// One banner, reused. Adding it again would stack a second copy in the viewport.
	if (!BannerWidget->IsInViewport())
	{
		BannerWidget->AddToPlayerScreen(ViewportZOrder);
	}

	BannerWidget->Show(Message);
}

void UBalhwajeomNotificationPresenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BannerWidget)
	{
		BannerWidget->RemoveFromParent();
		BannerWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}


void BalhwajeomNotification::Show(const UObject* WorldContextObject, const FText& Message)
{
	if (!WorldContextObject || Message.IsEmpty())
	{
		return;
	}

	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return;
	}

	if (UBalhwajeomNotificationPresenter* Presenter =
		PlayerController->FindComponentByClass<UBalhwajeomNotificationPresenter>())
	{
		Presenter->ShowNotification(Message);
	}
}
