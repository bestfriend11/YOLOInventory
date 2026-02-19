#pragma once

#include "CoreMinimal.h"
#include "Data/YIDataTableItemSource.h"

class FProperty;
class UObject;
class UYIItemDefinition;
class UScriptStruct;
class UStruct;

struct FYIResolvedMappingTarget
{
	FProperty* Property = nullptr;
	uint8* ValuePtr = nullptr;
	const UStruct* OwnerStruct = nullptr;
};

/** Returns the effective field name for destination writes (fragment field if present, otherwise TargetProperty). */
YOLOINVENTORY_API FName YIGetResolvedTargetFieldName(const FYIFieldMapping& Mapping);

/** Resolve a source row field by authored name and return property + data pointer. */
YOLOINVENTORY_API bool YIResolveMappingSource(
	const UScriptStruct* SourceStruct,
	const uint8* SourceData,
	FName SourceField,
	const FProperty*& OutProperty,
	const uint8*& OutValuePtr,
	FString* OutError = nullptr);

/** Resolve mapping destination on an arbitrary target object (legacy property + item static fragment layers). */
YOLOINVENTORY_API bool YIResolveMappingTarget(
	UObject* TargetObject,
	const FYIFieldMapping& Mapping,
	FYIResolvedMappingTarget& OutTarget,
	FString* OutError = nullptr);

/** Convenience overload for item definitions. */
YOLOINVENTORY_API bool YIResolveMappingTarget(
	UYIItemDefinition* ItemDefinition,
	const FYIFieldMapping& Mapping,
	FYIResolvedMappingTarget& OutTarget,
	FString* OutError = nullptr);
