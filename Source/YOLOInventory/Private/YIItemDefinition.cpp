#include "YIItemDefinition.h"
#include "YIItemRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
void UYIItemDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);
	if (UniqueCode == 0)
	{
		if (UEngine* Engine = GEngine)
		{
			if (UYIItemRegistrySubsystem* Sys = Engine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
			{
				Sys->BuildIndex(true);
				TSet<int64> Seen;
				// collect existing codes
				// Note: registry API provides map but not enumerator; rebuild index ensures keys present
				// We'll loop and generate until not in registry by checking GetByCode
				int64 NewCode = 0;
				do {
					NewCode = (int64)FMath::RandRange(100000, INT32_MAX) * 1000ll + (int64)FMath::RandRange(0,999);
				} while (Sys->GetByCode(NewCode) != nullptr);
				UniqueCode = NewCode;
			}
		}
	}
}
#endif
