#include "YIInlineMappingResolvers.h"

#include "YIAffixAsset.h"
#include "YIItemDefinition.h"
#include "YIItemFragments.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/UnrealType.h"

namespace
{
	static FProperty* YIFindPropertyByAuthoredName(const UStruct* OwnerStruct, FName FieldName)
	{
		if (!OwnerStruct || FieldName.IsNone())
		{
			return nullptr;
		}

		const FString FieldNameString = FieldName.ToString();
		for (TFieldIterator<FProperty> It(OwnerStruct); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property)
			{
				continue;
			}
			if (Property->GetAuthoredName().Equals(FieldNameString, ESearchCase::IgnoreCase))
			{
				return Property;
			}
		}
		return nullptr;
	}

	static void YISetResolveError(FString* OutError, const FString& Message)
	{
		if (OutError)
		{
			*OutError = Message;
		}
	}

	static bool YIResolvePropertyPathConst(
		const UStruct* RootStruct,
		const uint8* RootData,
		const FString& PathString,
		const FProperty*& OutProperty,
		const uint8*& OutValuePtr,
		FString* OutError)
	{
		OutProperty = nullptr;
		OutValuePtr = nullptr;

		if (!RootStruct || !RootData)
		{
			YISetResolveError(OutError, TEXT("Root struct/data is missing."));
			return false;
		}
		if (PathString.IsEmpty())
		{
			YISetResolveError(OutError, TEXT("Field path is empty."));
			return false;
		}

		TArray<FString> Segments;
		PathString.ParseIntoArray(Segments, TEXT("."), true);
		if (Segments.Num() == 0)
		{
			Segments.Add(PathString);
		}

		const UStruct* CurrentStruct = RootStruct;
		const uint8* CurrentData = RootData;
		for (int32 Index = 0; Index < Segments.Num(); ++Index)
		{
			const FName SegmentName(*Segments[Index]);
			FProperty* SegmentProp = YIFindPropertyByAuthoredName(CurrentStruct, SegmentName);
			if (!SegmentProp)
			{
				YISetResolveError(OutError, FString::Printf(TEXT("Field segment '%s' not found on '%s'."),
					*Segments[Index], *CurrentStruct->GetName()));
				return false;
			}

			const uint8* SegmentValuePtr = SegmentProp->ContainerPtrToValuePtr<uint8>(CurrentData);
			if (!SegmentValuePtr)
			{
				YISetResolveError(OutError, FString::Printf(TEXT("Field segment '%s' has invalid data pointer."), *Segments[Index]));
				return false;
			}

			const bool bLastSegment = (Index == Segments.Num() - 1);
			if (bLastSegment)
			{
				OutProperty = SegmentProp;
				OutValuePtr = SegmentValuePtr;
				return true;
			}

			const FStructProperty* StructProp = CastField<FStructProperty>(SegmentProp);
			if (!StructProp || !StructProp->Struct)
			{
				YISetResolveError(OutError, FString::Printf(TEXT("Field segment '%s' is not a struct."), *Segments[Index]));
				return false;
			}

			CurrentStruct = StructProp->Struct;
			CurrentData = SegmentValuePtr;
		}

		return false;
	}

	static bool YIResolvePropertyPathMutable(
		const UStruct* RootStruct,
		uint8* RootData,
		const FString& PathString,
		FProperty*& OutProperty,
		uint8*& OutValuePtr,
		FString* OutError)
	{
		OutProperty = nullptr;
		OutValuePtr = nullptr;

		const FProperty* ConstProp = nullptr;
		const uint8* ConstPtr = nullptr;
		if (!YIResolvePropertyPathConst(RootStruct, RootData, PathString, ConstProp, ConstPtr, OutError))
		{
			return false;
		}

		OutProperty = const_cast<FProperty*>(ConstProp);
		OutValuePtr = const_cast<uint8*>(ConstPtr);
		return true;
	}
}

FName YIGetResolvedTargetFieldName(const FYIFieldMapping& Mapping)
{
	return Mapping.TargetFragmentField.IsNone() ? Mapping.TargetProperty : Mapping.TargetFragmentField;
}

bool YIResolveMappingSource(
	const UScriptStruct* SourceStruct,
	const uint8* SourceData,
	FName SourceField,
	const FProperty*& OutProperty,
	const uint8*& OutValuePtr,
	FString* OutError)
{
	OutProperty = nullptr;
	OutValuePtr = nullptr;

	if (!SourceStruct || !SourceData)
	{
		YISetResolveError(OutError, TEXT("Source struct/data is missing."));
		return false;
	}
	if (SourceField.IsNone())
	{
		YISetResolveError(OutError, TEXT("Source field is not set."));
		return false;
	}
	return YIResolvePropertyPathConst(SourceStruct, SourceData, SourceField.ToString(), OutProperty, OutValuePtr, OutError);
}

bool YIResolveMappingTarget(
	UYIItemDefinition* ItemDefinition,
	const FYIFieldMapping& Mapping,
	FYIResolvedMappingTarget& OutTarget,
	FString* OutError)
{
	return YIResolveMappingTarget(static_cast<UObject*>(ItemDefinition), Mapping, OutTarget, OutError);
}

bool YIResolveMappingTarget(
	UObject* TargetObject,
	const FYIFieldMapping& Mapping,
	FYIResolvedMappingTarget& OutTarget,
	FString* OutError)
{
	OutTarget = FYIResolvedMappingTarget();

	if (!TargetObject)
	{
		YISetResolveError(OutError, TEXT("Target object is null."));
		return false;
	}

	if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty)
	{
		if (Mapping.TargetProperty.IsNone())
		{
			YISetResolveError(OutError, TEXT("Target property is not set."));
			return false;
		}

		FProperty* TargetProperty = nullptr;
		uint8* TargetValuePtr = nullptr;
		if (!YIResolvePropertyPathMutable(TargetObject->GetClass(), reinterpret_cast<uint8*>(TargetObject), Mapping.TargetProperty.ToString(), TargetProperty, TargetValuePtr, OutError))
		{
			return false;
		}

		OutTarget.Property = TargetProperty;
		OutTarget.ValuePtr = TargetValuePtr;
		OutTarget.OwnerStruct = TargetObject->GetClass();
		return OutTarget.ValuePtr != nullptr;
	}

	if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::DynamicInstanceFragment)
	{
		YISetResolveError(OutError, TEXT("Dynamic instance fragment targets are not valid for definition/asset mapping."));
		return false;
	}

	const UScriptStruct* FragmentStruct = Mapping.TargetFragmentStruct.Get();
	if (!FragmentStruct)
	{
		YISetResolveError(OutError, TEXT("Target fragment struct is not set."));
		return false;
	}

	const FName FragmentField = YIGetResolvedTargetFieldName(Mapping);
	if (FragmentField.IsNone())
	{
		YISetResolveError(OutError, TEXT("Target fragment field is not set."));
		return false;
	}

	FInstancedStruct* Fragment = nullptr;
	if (UYIItemDefinition* ItemDefinition = Cast<UYIItemDefinition>(TargetObject))
	{
		if (!FragmentStruct->IsChildOf(FYIItemDefinitionFragmentBase::StaticStruct()))
		{
			YISetResolveError(OutError, FString::Printf(TEXT("Fragment struct '%s' must derive from FYIItemDefinitionFragmentBase."),
				*FragmentStruct->GetName()));
			return false;
		}
		Fragment = ItemDefinition->FindOrAddDefinitionFragmentByStruct(FragmentStruct);
	}
	else if (UYIAffixAsset* AffixAsset = Cast<UYIAffixAsset>(TargetObject))
	{
		if (!FragmentStruct->IsChildOf(FYIAffixDefinitionFragmentBase::StaticStruct()))
		{
			YISetResolveError(OutError, FString::Printf(TEXT("Fragment struct '%s' must derive from FYIAffixDefinitionFragmentBase."),
				*FragmentStruct->GetName()));
			return false;
		}
		Fragment = AffixAsset->FindOrAddDefinitionFragmentByStruct(FragmentStruct);
	}
	else
	{
		YISetResolveError(OutError, TEXT("Static fragment target requires UYIItemDefinition or UYIAffixAsset."));
		return false;
	}

	if (!Fragment || !Fragment->IsValid())
	{
		YISetResolveError(OutError, TEXT("Unable to create/resolve target fragment instance."));
		return false;
	}

	FProperty* TargetProperty = nullptr;
	uint8* TargetValuePtr = nullptr;
	if (!YIResolvePropertyPathMutable(FragmentStruct, Fragment->GetMutableMemory(), FragmentField.ToString(), TargetProperty, TargetValuePtr, OutError))
	{
		return false;
	}

	OutTarget.Property = TargetProperty;
	OutTarget.ValuePtr = TargetValuePtr;
	OutTarget.OwnerStruct = FragmentStruct;
	return OutTarget.ValuePtr != nullptr;
}
