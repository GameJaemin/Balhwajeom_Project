#include "Core/ItemInspectorSettings.h"

UItemInspectorSettings::UItemInspectorSettings()
{
	CategoryName = TEXT("JM Plugins");
	SectionName = TEXT("Item Inspector");
	DefaultInspectorWidgetClass = TSoftClassPtr<UJMItemInspectionWidgetBase>(FSoftObjectPath(TEXT("/ItemInspector/UI/WBP_JMItemInspection.WBP_JMItemInspection_C")));
}

const UItemInspectorSettings* UItemInspectorSettings::Get()
{
	return GetDefault<UItemInspectorSettings>();
}

FName UItemInspectorSettings::GetCategoryName() const
{
	return TEXT("JM Plugins");
}
