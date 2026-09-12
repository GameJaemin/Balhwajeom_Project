#include "Settings/JMInteractionSettings.h"

#include "UI/JMInteractionPromptWidgetBase.h"

UJMInteractionSettings::UJMInteractionSettings()
{
    CategoryName = TEXT("JM Plugins");
    SectionName = TEXT("Interaction");
}

const UJMInteractionSettings* UJMInteractionSettings::Get()
{
    return GetDefault<UJMInteractionSettings>();
}

FName UJMInteractionSettings::GetCategoryName() const
{
    return TEXT("JM Plugins");
}
