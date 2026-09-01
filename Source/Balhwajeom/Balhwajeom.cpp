// Copyright Epic Games, Inc. All Rights Reserved.

#include "Balhwajeom.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"
#include "Modules/ModuleManager.h"

namespace
{
	void AddActionMappingIfMissing(UInputSettings* Settings, const FInputActionKeyMapping& Mapping)
	{
		if (!Settings->GetActionMappings().Contains(Mapping))
		{
			Settings->AddActionMapping(Mapping, false);
		}
	}

	void AddAxisMappingIfMissing(UInputSettings* Settings, const FInputAxisKeyMapping& Mapping)
	{
		if (!Settings->GetAxisMappings().Contains(Mapping))
		{
			Settings->AddAxisMapping(Mapping, false);
		}
	}
}

class FBalhwajeomGameModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		UInputSettings* Settings = GetMutableDefault<UInputSettings>();
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("CameraMode"), EKeys::RightMouseButton));
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("ExitCameraMode"), EKeys::Escape));
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("TakePhoto"), EKeys::LeftMouseButton));
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("Sprint"), EKeys::LeftShift));
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("Sprint"), EKeys::RightShift));
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("Jump"), EKeys::SpaceBar));
		AddActionMappingIfMissing(Settings, FInputActionKeyMapping(TEXT("Jump"), EKeys::Gamepad_FaceButton_Bottom));

		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("MoveForward"), EKeys::W, 1.0f));
		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("MoveForward"), EKeys::S, -1.0f));
		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("MoveRight"), EKeys::D, 1.0f));
		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("MoveRight"), EKeys::A, -1.0f));
		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("LookUp"), EKeys::MouseY, -1.0f));
		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("Turn"), EKeys::MouseX, 1.0f));
		AddAxisMappingIfMissing(Settings, FInputAxisKeyMapping(TEXT("CameraZoom"), EKeys::MouseWheelAxis, 1.0f));
		Settings->ForceRebuildKeymaps();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FBalhwajeomGameModule, Balhwajeom, "Balhwajeom");

DEFINE_LOG_CATEGORY(LogBalhwajeom)
