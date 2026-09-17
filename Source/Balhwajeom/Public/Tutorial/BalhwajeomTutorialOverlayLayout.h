#pragma once

#include "CoreMinimal.h"

class UHorizontalBox;
class UScaleBox;
class USizeBox;
class UTextBlock;
class UWidget;
class UWidgetTree;


namespace BalhwajeomTutorialOverlayLayout
{
	/** Widget names the overlay binds to. Shared so the editor builder cannot drift from BindWidget. */
	namespace WidgetNames
	{
		inline const TCHAR* RootCanvas = TEXT("RootCanvas");
		inline const TCHAR* RootOverlay = TEXT("OVL_Root");
		inline const TCHAR* BackgroundBlur = TEXT("BG_Blur");
		inline const TCHAR* Dim = TEXT("BRD_Dim");
		inline const TCHAR* ContentBox = TEXT("SB_Content");
		inline const TCHAR* ContentColumn = TEXT("VB_Content");
		inline const TCHAR* ImageScale = TEXT("SCB_Images");
		inline const TCHAR* ImageRow = TEXT("HB_Images");
		inline const TCHAR* Title = TEXT("TXT_Title");
		inline const TCHAR* DescriptionBox = TEXT("SB_Description");
		inline const TCHAR* Description = TEXT("TXT_Description");
		inline const TCHAR* ContinuePrompt = TEXT("TXT_ContinuePrompt");
	}

	/** The widgets the runtime class drives. Everything else is pure presentation. */
	struct FBoundWidgets
	{
		UScaleBox* ImageScale = nullptr;
		UHorizontalBox* ImageRow = nullptr;
		UTextBlock* Title = nullptr;
		USizeBox* DescriptionBox = nullptr;
		UTextBlock* Description = nullptr;
		UTextBlock* ContinuePrompt = nullptr;

		bool IsComplete() const
		{
			return ImageScale && ImageRow && Title && DescriptionBox && Description &&
				ContinuePrompt;
		}

		/** Iteration order matches the authored column, top to bottom. */
		BALHWAJEOM_API TArray<TPair<const TCHAR*, UWidget*>> AsNamedPairs() const;
	};

	/** Design-space width of the centred content column. */
	inline constexpr float ContentWidth = 800.0f;

	/** Widest the description may wrap to. Short text stays narrow and therefore centred. */
	inline constexpr float DescriptionMaxWidth = 800.0f;

	/**
	 * Builds the whole overlay tree into Tree and assigns Tree.RootWidget.
	 *
	 * The editor builder writes this into WBP_TutorialOverlay and the runtime class falls back
	 * to it when instantiated without a Widget Blueprint, so both paths lay out identically.
	 * Fails without clearing anything if Tree already has a root.
	 */
	BALHWAJEOM_API bool Build(UWidgetTree& Tree, FBoundWidgets& OutWidgets);
}
