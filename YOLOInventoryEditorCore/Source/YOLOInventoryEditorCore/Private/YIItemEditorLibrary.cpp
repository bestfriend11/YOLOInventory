#include "YIItemEditorLibrary.h"
#include "Engine/Engine.h"
#include "Subsystems/EngineSubsystem.h"

#if WITH_EDITOR
bool UYIItemEditorLibrary::EnsureUniqueCodes(bool bAutoFix)
{
	if (UEngine* Engine = GEngine)
	{
		UClass* RegistryClass = LoadClass<UEngineSubsystem>(nullptr, TEXT("/Script/YOLOInventorySchema.YIItemRegistrySubsystem"));
		if (!RegistryClass)
		{
			return false;
		}

		if (UEngineSubsystem* Sys = Engine->GetEngineSubsystemBase(RegistryClass))
		{
			if (UFunction* EnsureFn = Sys->FindFunction(TEXT("EnsureUniqueCodes")))
			{
				struct FEnsureUniqueCodesParams
				{
					bool bAutoFix;
					bool ReturnValue;
				};
				FEnsureUniqueCodesParams Params{ bAutoFix, false };
				Sys->ProcessEvent(EnsureFn, &Params);
				return Params.ReturnValue;
			}
		}
	}
	return false;
}
#endif
