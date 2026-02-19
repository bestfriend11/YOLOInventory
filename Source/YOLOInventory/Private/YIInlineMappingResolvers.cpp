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

	FProperty* SourceProperty = YIFindPropertyByAuthoredName(SourceStruct, SourceField);
	if (!SourceProperty)
	{
		YISetResolveError(OutError, FString::Printf(TEXT("Source field '%s' not found."), *SourceField.ToString()));
		return false;
	}

	OutProperty = SourceProperty;
	OutValuePtr = SourceProperty->ContainerPtrToValuePtr<uint8>(SourceData);
	return OutValuePtr != nullptr;
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

		FProperty* TargetProperty = YIFindPropertyByAuthoredName(TargetObject->GetClass(), Mapping.TargetProperty);
		if (!TargetProperty)
		{
			YISetResolveError(OutError, FString::Printf(TEXT("Target property '%s' not found on '%s'."),
				*Mapping.TargetProperty.ToString(), *TargetObject->GetClass()->GetName()));
			return false;
		}

		OutTarget.Property = TargetProperty;
		OutTarget.ValuePtr = TargetProperty->ContainerPtrToValuePtr<uint8>(TargetObject);
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

	FProperty* TargetProperty = YIFindPropertyByAuthoredName(FragmentStruct, FragmentField);
	if (!TargetProperty)
	{
		YISetResolveError(OutError, FString::Printf(TEXT("Field '%s' not found on fragment '%s'."),
			*FragmentField.ToString(), *FragmentStruct->GetName()));
		return false;
	}

	OutTarget.Property = TargetProperty;
	OutTarget.ValuePtr = TargetProperty->ContainerPtrToValuePtr<uint8>(Fragment->GetMutableMemory());
	OutTarget.OwnerStruct = FragmentStruct;
	return OutTarget.ValuePtr != nullptr;
}
