#include "UI/BalhwajeomMainMenuWidget.h"

#include "Components/Button.h"

void UBalhwajeomMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bStartAccepted = false;
	if (BTN_Start)
	{
		BTN_Start->OnClicked.RemoveAll(this);
		BTN_Start->OnClicked.AddDynamic(this, &ThisClass::HandleStartClicked);
		BTN_Start->SetIsEnabled(true);
	}
}

void UBalhwajeomMainMenuWidget::NativeDestruct()
{
	if (BTN_Start)
	{
		BTN_Start->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UBalhwajeomMainMenuWidget::SetStartButtonEnabled(bool bEnabled)
{
	bStartAccepted = !bEnabled;
	if (BTN_Start)
	{
		BTN_Start->SetIsEnabled(bEnabled);
	}
}

void UBalhwajeomMainMenuWidget::HandleStartClicked()
{
	if (bStartAccepted)
	{
		return;
	}
	bStartAccepted = true;
	if (BTN_Start)
	{
		BTN_Start->SetIsEnabled(false);
	}
	OnStartRequested.Broadcast();
}
