#include "UI/BalhwajeomCinematicVideoWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "MediaTexture.h"

void UBalhwajeomCinematicVideoWidget::SetMediaTexture(UMediaTexture* MediaTexture)
{
	if (!IMG_Video || !MediaTexture)
	{
		return;
	}

	FSlateBrush Brush = IMG_Video->GetBrush();
	Brush.SetResourceObject(MediaTexture);
	Brush.ImageSize = FVector2D(1920.0f, 1080.0f);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	IMG_Video->SetBrush(Brush);
}

void UBalhwajeomCinematicVideoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BTN_Skip)
	{
		BTN_Skip->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSkipClicked);
	}
}

void UBalhwajeomCinematicVideoWidget::HandleSkipClicked()
{
	OnSkipRequested.Broadcast();
}
