#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Editor.h"
#include "Tablet/BalhwajeomMessengerWidget.h"
#include "WidgetBlueprint.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomMessengerStaticWidgetTest,
	"Balhwajeom.Tablet.Messenger.StaticRoomSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomMessengerStaticWidgetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TCHAR* AssetPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_Messenger.WBP_Messenger");
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
	TestNotNull(TEXT("Static messenger WBP exists"), Blueprint);
	if (!Blueprint || !Blueprint->WidgetTree)
	{
		return false;
	}

	for (const TCHAR* ButtonName : {
		TEXT("BTN_RoomDad"),
		TEXT("BTN_RoomMother"),
		TEXT("BTN_RoomSister"),
		TEXT("BTN_RoomBrother"),
		TEXT("BTN_Back")})
	{
		TestNotNull(
			FString::Printf(TEXT("Button exists: %s"), ButtonName),
			Cast<UButton>(Blueprint->WidgetTree->FindWidget(ButtonName)));
	}
	for (const TCHAR* ImageName : {
		TEXT("IMG_RoomDad"),
		TEXT("IMG_RoomMother"),
		TEXT("IMG_RoomSister"),
		TEXT("IMG_RoomBrother")})
	{
		TestNotNull(
			FString::Printf(TEXT("Room artwork exists: %s"), ImageName),
			Cast<UImage>(Blueprint->WidgetTree->FindWidget(ImageName)));
	}
	TestNotNull(
		TEXT("Selection artwork exists"),
		Cast<UImage>(Blueprint->WidgetTree->FindWidget(TEXT("IMG_RoomSelection"))));

	const USizeBox* DesignSize = Cast<USizeBox>(
		Blueprint->WidgetTree->FindWidget(TEXT("SizeBox_Wrapper")));
	TestNotNull(TEXT("Fixed messenger design size exists"), DesignSize);
	if (DesignSize)
	{
		TestEqual(TEXT("Messenger design width"), DesignSize->GetWidthOverride(), 650.0f);
		TestEqual(TEXT("Messenger design height"), DesignSize->GetHeightOverride(), 699.0f);
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Editor world exists"), World);
	UClass* WidgetClass = Blueprint->GeneratedClass;
	UBalhwajeomMessengerWidget* Messenger = World && WidgetClass
		? CreateWidget<UBalhwajeomMessengerWidget>(World, WidgetClass)
		: nullptr;
	TestNotNull(TEXT("Static messenger can be instantiated"), Messenger);
	if (!Messenger)
	{
		return false;
	}

	Messenger->InitializeForAutomatedTest();
	TestEqual(TEXT("Default room is Dad"), Messenger->GetCurrentRoomID(), FString(TEXT("Dad")));
	TestEqual(TEXT("Four static rooms are displayed"), Messenger->GetDisplayedRoomCount(), 4);
	TestEqual(TEXT("Conversation area is empty"), Messenger->GetDisplayedMessageCount(), 0);
	TestEqual(TEXT("Legacy unread state is disabled"), Messenger->GetTotalUnreadCount(), 0);

	for (const TCHAR* RoomID : {TEXT("Mother"), TEXT("Sister"), TEXT("Brother"), TEXT("Dad")})
	{
		TestTrue(
			FString::Printf(TEXT("Room can be selected: %s"), RoomID),
			Messenger->SelectRoomByID(RoomID));
		TestEqual(
			FString::Printf(TEXT("Selected room is stored: %s"), RoomID),
			Messenger->GetCurrentRoomID(),
			FString(RoomID));
	}

	TestFalse(TEXT("Unknown room is rejected"), Messenger->SelectRoomByID(TEXT("Unknown")));
	TestEqual(TEXT("Rejected selection preserves current room"), Messenger->GetCurrentRoomID(), FString(TEXT("Dad")));
	return true;
}

#endif
