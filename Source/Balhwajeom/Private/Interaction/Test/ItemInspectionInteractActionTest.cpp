#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Interaction/ItemInspectionIntegration.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FItemInspectionInteractActionTest,
	"Balhwajeom.Interaction.ItemInspection.InteractAction",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FItemInspectionInteractActionTest::RunTest(const FString& Parameters)
{
	// The rotating inspector hears F through its own widget focus, and a click the game viewport
	// captures takes that focus away. When the key arrives at gameplay instead, it has to close the
	// inspector rather than be refused, otherwise the player is stuck in a view with no way out.
	TestEqual(
		TEXT("F closes an inspector the player is looking at"),
		BalhwajeomItemInspection::ResolveInteractAction(true, true, false),
		EBalhwajeomInteractAction::CloseInspection);
	TestEqual(
		TEXT("An open inspector wins over another modal"),
		BalhwajeomItemInspection::ResolveInteractAction(true, true, true),
		EBalhwajeomInteractAction::CloseInspection);

	// A pawn can carry two interaction components - BP_OrbitViewCharacter_Legacy adds
	// BPC_PlayerInteraction on top of the one ABalhwajeomCameraCharacter creates - so one F press
	// reaches this twice in the same frame. Closing on the second pass would kill the inspector
	// during its entrance transition, before the model was ever drawn.
	TestEqual(
		TEXT("F does not close an inspector that is still opening"),
		BalhwajeomItemInspection::ResolveInteractAction(true, false, false),
		EBalhwajeomInteractAction::None);
	TestEqual(
		TEXT("F is refused while the tablet or photo camera owns the screen"),
		BalhwajeomItemInspection::ResolveInteractAction(false, false, true),
		EBalhwajeomInteractAction::None);
	TestEqual(
		TEXT("F interacts normally with nothing on screen"),
		BalhwajeomItemInspection::ResolveInteractAction(false, false, false),
		EBalhwajeomInteractAction::Interact);
	return true;
}

#endif
