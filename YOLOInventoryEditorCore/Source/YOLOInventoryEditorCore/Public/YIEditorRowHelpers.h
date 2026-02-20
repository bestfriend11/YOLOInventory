#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

inline FString YIEditor_GetRowStringFromStruct(const UScriptStruct* Struct, const uint8* RowData, FName Field)
{
	if (!Struct || !RowData || Field.IsNone())
	{
		return FString();
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!Prop)
		{
			continue;
		}
		const FString Authored = Prop->GetAuthoredName();
		if (!Authored.Equals(Field.ToString(), ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (const FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			return StrProp->GetPropertyValue_InContainer(RowData);
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			return NameProp->GetPropertyValue_InContainer(RowData).ToString();
		}
		if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop))
		{
			return TextProp->GetPropertyValue_InContainer(RowData).ToString();
		}
	}
	return FString();
}
