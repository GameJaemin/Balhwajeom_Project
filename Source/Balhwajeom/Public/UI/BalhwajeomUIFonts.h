#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"


/**
 * The project's Korean UI faces, in one place.
 *
 * These used to be copied into an anonymous namespace per widget file, which is fine until
 * two such files land in the same unity translation unit -- the anonymous namespaces merge
 * and the duplicated names collide. The editor target hid it, because adaptive unity keeps
 * recently edited files out of the blob; the game target compiled them together and broke.
 */
namespace BalhwajeomUIFonts
{
	BALHWAJEOM_API extern const TCHAR* const RegularPath;
	BALHWAJEOM_API extern const TCHAR* const SemiBoldPath;

	/**
	 * InheritedFont with Size applied and the Korean face swapped in.
	 *
	 * A missing face keeps the inherited one rather than losing the size, so a font that
	 * failed to load shows as the wrong typeface instead of the wrong layout.
	 * A null FontPath means RegularPath.
	 */
	BALHWAJEOM_API FSlateFontInfo MakeKoreanFont(
		const FSlateFontInfo& InheritedFont,
		int32 Size,
		const TCHAR* FontPath = nullptr);
}
