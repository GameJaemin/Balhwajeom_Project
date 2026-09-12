#include "UI/BalhwajeomCinematicVideoWidget.h"

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
