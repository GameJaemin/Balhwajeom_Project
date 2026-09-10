#include "Animation/DogAnimationBlueprintLibrary.h"

#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/DogAnimInstance.h"
#include "AnimationGraph.h"
#include "AssetToolsModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Factories/AnimBlueprintFactory.h"
#include "IAssetTools.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace DogAnimationBlueprint
{
	constexpr const TCHAR* AssetFolder = TEXT("/Game/Balhwajeom/Characters/Player");
	constexpr const TCHAR* AssetName = TEXT("ABP_Dog");
	constexpr const TCHAR* AssetPath = TEXT("/Game/Balhwajeom/Characters/Player/ABP_Dog.ABP_Dog");
	constexpr const TCHAR* IdlePath = TEXT("/Game/Balhwajeom/Characters/Player/Dog_Idle_Anim.Dog_Idle_Anim");
	constexpr const TCHAR* WalkPath = TEXT("/Game/Balhwajeom/Characters/Player/Dog_Walking_Anim.Dog_Walking_Anim");

	template <typename NodeType>
	NodeType* AddNode(UEdGraph* Graph, const int32 X, const int32 Y)
	{
		NodeType* Node = NewObject<NodeType>(Graph);
		Graph->AddNode(Node, true, false);
		Node->CreateNewGuid();
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		return Node;
	}

	bool Connect(UEdGraph* Graph, UEdGraphPin* Output, UEdGraphPin* Input, const TCHAR* Label)
	{
		if (!Graph || !Output || !Input || !Graph->GetSchema()->TryCreateConnection(Output, Input))
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_DOG: failed to connect %s"), Label);
			return false;
		}
		return true;
	}

	UEdGraphPin* FindPinChecked(UEdGraphNode* Node, const FName PinName)
	{
		if (UEdGraphPin* Pin = Node ? Node->FindPin(PinName) : nullptr)
		{
			return Pin;
		}
		if (Node)
		{
			FString Available;
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				Available += FString::Printf(TEXT(" %s"), *Pin->PinName.ToString());
			}
			UE_LOG(LogTemp, Error, TEXT("ABP_DOG: pin '%s' missing on %s. Available:%s"),
				*PinName.ToString(), *Node->GetClass()->GetName(), *Available);
		}
		return nullptr;
	}

	bool Save(UAnimBlueprint* Blueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection);
		if (Blueprint->Status == BS_Error)
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_DOG: Blueprint compilation failed"));
			return false;
		}

		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.bSlowTask = false;
		return UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
	}
}

bool UDogAnimationBlueprintLibrary::CreateDogAnimationBlueprint()
{
	using namespace DogAnimationBlueprint;

	if (LoadObject<UAnimBlueprint>(nullptr, AssetPath))
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG: %s already exists; refusing to overwrite designer work"), AssetPath);
		return false;
	}

	UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, IdlePath);
	UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, WalkPath);
	if (!Idle || !Walk || !Idle->GetSkeleton() || Idle->GetSkeleton() != Walk->GetSkeleton())
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG: Idle/Walk assets are missing or use different skeletons"));
		return false;
	}

	UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->ParentClass = UDogAnimInstance::StaticClass();
	Factory->TargetSkeleton = Idle->GetSkeleton();
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(AssetTools.CreateAsset(
		AssetName, AssetFolder, UAnimBlueprint::StaticClass(), Factory));
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG: failed to create asset"));
		return false;
	}

	UAnimationGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (UAnimationGraph* Candidate = Cast<UAnimationGraph>(Graph))
		{
			AnimGraph = Candidate;
			break;
		}
	}
	if (!AnimGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG: AnimGraph was not created"));
		return false;
	}

	TArray<UAnimGraphNode_Root*> RootNodes;
	AnimGraph->GetNodesOfClass(RootNodes);
	if (RootNodes.Num() != 1)
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG: expected one output pose node, found %d"), RootNodes.Num());
		return false;
	}
	UAnimGraphNode_Root* Root = RootNodes[0];
	Root->NodePosX = 650;
	Root->NodePosY = 0;

	UAnimGraphNode_SequencePlayer* IdleNode = AddNode<UAnimGraphNode_SequencePlayer>(AnimGraph, -650, 180);
	IdleNode->SetAnimationAsset(Idle);
	IdleNode->Node.SetLoopAnimation(true);
	UAnimGraphNode_SequencePlayer* WalkNode = AddNode<UAnimGraphNode_SequencePlayer>(AnimGraph, -650, -120);
	WalkNode->SetAnimationAsset(Walk);
	WalkNode->Node.SetLoopAnimation(true);

	UAnimGraphNode_BlendListByBool* BlendNode = AddNode<UAnimGraphNode_BlendListByBool>(AnimGraph, 100, 0);
	UK2Node_VariableGet* MovingNode = NewObject<UK2Node_VariableGet>(AnimGraph);
	AnimGraph->AddNode(MovingNode, true, false);
	MovingNode->CreateNewGuid();
	MovingNode->PostPlacedNewNode();
	FProperty* MovingProperty = FindFProperty<FProperty>(UDogAnimInstance::StaticClass(), GET_MEMBER_NAME_CHECKED(UDogAnimInstance, bIsMoving));
	MovingNode->SetFromProperty(MovingProperty, true, UDogAnimInstance::StaticClass());
	MovingNode->AllocateDefaultPins();
	MovingNode->NodePosX = -350;
	MovingNode->NodePosY = -360;

	bool bConnected = true;
	bConnected &= Connect(AnimGraph, FindPinChecked(WalkNode, TEXT("Pose")), FindPinChecked(BlendNode, TEXT("BlendPose_0")), TEXT("Walk -> True Pose"));
	bConnected &= Connect(AnimGraph, FindPinChecked(IdleNode, TEXT("Pose")), FindPinChecked(BlendNode, TEXT("BlendPose_1")), TEXT("Idle -> False Pose"));
	bConnected &= Connect(AnimGraph, MovingNode->GetValuePin(), FindPinChecked(BlendNode, TEXT("bActiveValue")), TEXT("IsMoving -> Active Value"));
	bConnected &= Connect(AnimGraph, FindPinChecked(BlendNode, TEXT("Pose")), FindPinChecked(Root, TEXT("Result")), TEXT("Blend -> Output Pose"));
	if (UEdGraphPin* TrueBlendTime = FindPinChecked(BlendNode, TEXT("BlendTime_0")))
	{
		TrueBlendTime->DefaultValue = TEXT("0.2");
	}
	if (UEdGraphPin* FalseBlendTime = FindPinChecked(BlendNode, TEXT("BlendTime_1")))
	{
		FalseBlendTime->DefaultValue = TEXT("0.2");
	}
	if (!bConnected || !Save(Blueprint))
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("ABP_DOG_CREATE Result=Success Asset=%s Idle=%s Walk=%s"), AssetPath, IdlePath, WalkPath);
	return ValidateDogAnimationBlueprint();
}

bool UDogAnimationBlueprintLibrary::ValidateDogAnimationBlueprint()
{
	using namespace DogAnimationBlueprint;
	UAnimBlueprint* Blueprint = LoadObject<UAnimBlueprint>(nullptr, AssetPath);
	if (!Blueprint || Blueprint->Status == BS_Error || !Blueprint->GeneratedClass ||
		!Blueprint->GeneratedClass->IsChildOf(UDogAnimInstance::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG_VALIDATE: asset is missing, invalid, or has the wrong parent"));
		return false;
	}

	TArray<UAnimGraphNode_SequencePlayer*> Players;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Players);
	TSet<const UAnimationAsset*> Sequences;
	for (const UAnimGraphNode_SequencePlayer* Player : Players)
	{
		Sequences.Add(Player->GetAnimationAsset());
	}
	UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, IdlePath);
	UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, WalkPath);
	TArray<UAnimGraphNode_BlendListByBool*> BlendNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, BlendNodes);
	const bool bValid = Players.Num() == 2 && BlendNodes.Num() == 1 && Sequences.Contains(Idle) && Sequences.Contains(Walk);
	if (bValid)
	{
		UE_LOG(LogTemp, Display, TEXT("ABP_DOG_VALIDATE Result=Success Players=%d BoolBlends=%d Parent=%s"),
			Players.Num(), BlendNodes.Num(), *Blueprint->GeneratedClass->GetSuperClass()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_DOG_VALIDATE Result=Failure Players=%d BoolBlends=%d Parent=%s"),
			Players.Num(), BlendNodes.Num(), *Blueprint->GeneratedClass->GetSuperClass()->GetName());
	}
	return bValid;
}
