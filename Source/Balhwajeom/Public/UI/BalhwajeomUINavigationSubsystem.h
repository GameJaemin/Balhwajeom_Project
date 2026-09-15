#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BalhwajeomUINavigationSubsystem.generated.h"


/**
 * Removes Tab from Slate's built-in UI focus navigation (its default "select next widget"
 * key) so IA_Tablet can use Tab as a plain gameplay hotkey without also cycling keyboard
 * focus between buttons on screen.
 *
 * A GameInstanceSubsystem is used purely as a guaranteed-once startup hook; the effect
 * itself is global and has nothing to do with GameInstance state.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomUINavigationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
