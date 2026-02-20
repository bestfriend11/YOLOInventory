#include "YIItemEditorLibrary.h"
#include "YIItemRegistrySubsystem.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
bool UYIItemEditorLibrary::EnsureUniqueCodes(bool bAutoFix)
{
	if (UEngine* Engine = GEngine)
	{
		if (UYIItemRegistrySubsystem* Sys = Engine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			return Sys->EnsureUniqueCodes(bAutoFix);
		}
	}
	return false;
}
#endif
