#include "UI/BalhwajeomUIFonts.h"

#include "Engine/Font.h"


const TCHAR* const BalhwajeomUIFonts::RegularPath =
	TEXT("/Game/Balhwajeom/UI/JE/Freesentation-4Regular_Font.Freesentation-4Regular_Font");

const TCHAR* const BalhwajeomUIFonts::SemiBoldPath =
	TEXT("/Game/Balhwajeom/UI/JE/Freesentation-6SemiBold_Font.Freesentation-6SemiBold_Font");


FSlateFontInfo BalhwajeomUIFonts::MakeKoreanFont(
	const FSlateFontInfo& InheritedFont,
	const int32 Size,
	const TCHAR* FontPath)
{
	FSlateFontInfo Font = InheritedFont;
	Font.Size = Size;

	if (UFont* KoreanFont = LoadObject<UFont>(nullptr, FontPath ? FontPath : RegularPath))
	{
		Font.FontObject = KoreanFont;
	}
	return Font;
}
