#include "YIInventoryUIScreenLibrary.h"

#include "InventoryScreenWidget.h"
#include "YIInventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	struct FYIInventoryScreenBindParams
	{
		UYIInventoryComponent* InInventoryComponent = nullptr;
	};

	static TMap<TWeakObjectPtr<UYIInventoryComponent>, TWeakObjectPtr<UUserWidget>> GInventoryScreensByComponent;

	static void BindInventoryScreenWidget(UUserWidget* Widget, UYIInventoryComponent* InventoryComponent)
	{
		if (!Widget || !InventoryComponent)
		{
			return;
		}

		if (UFunction* Fn = Widget->FindFunction(TEXT("BindInventoryBagContexts")))
		{
			FYIInventoryScreenBindParams Params;
			Params.InInventoryComponent = InventoryComponent;
			Widget->ProcessEvent(Fn, &Params);
		}
	}

	static APlayerController* ResolveLocalOwningPlayerController(UYIInventoryComponent* InventoryComponent)
	{
		if (!InventoryComponent)
		{
			return nullptr;
		}

		AActor* Owner = InventoryComponent->GetOwner();
		if (!Owner || Owner->GetNetMode() == NM_DedicatedServer)
		{
			return nullptr;
		}

		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				return PC->IsLocalController() ? PC : nullptr;
			}
			return nullptr;
		}

		if (APlayerController* PC = Cast<APlayerController>(Owner))
		{
			return PC->IsLocalController() ? PC : nullptr;
		}

		return nullptr;
	}
}

UUserWidget* UYIInventoryUIScreenLibrary::OpenInventoryScreenForComponent(UObject* WorldContextObject, UYIInventoryComponent* InventoryComponent, TSubclassOf<UUserWidget> ScreenClass)
{
	(void)WorldContextObject;
	if (!InventoryComponent)
	{
		return nullptr;
	}

	if (TWeakObjectPtr<UUserWidget>* ExistingPtr = GInventoryScreensByComponent.Find(InventoryComponent))
	{
		if (UUserWidget* Existing = ExistingPtr->Get())
		{
			BindInventoryScreenWidget(Existing, InventoryComponent);
			return Existing;
		}
		GInventoryScreensByComponent.Remove(InventoryComponent);
	}

	APlayerController* PC = ResolveLocalOwningPlayerController(InventoryComponent);
	if (!PC)
	{
		return nullptr;
	}

	UClass* ResolvedClass = ScreenClass ? *ScreenClass : UInventoryScreenWidget::StaticClass();
	if (!ResolvedClass)
	{
		return nullptr;
	}

	UUserWidget* Screen = CreateWidget<UUserWidget>(PC, ResolvedClass);
	if (!Screen)
	{
		return nullptr;
	}

	BindInventoryScreenWidget(Screen, InventoryComponent);
	Screen->AddToViewport();
	GInventoryScreensByComponent.Add(InventoryComponent, Screen);
	return Screen;
}

void UYIInventoryUIScreenLibrary::CloseInventoryScreenForComponent(UYIInventoryComponent* InventoryComponent)
{
	if (!InventoryComponent)
	{
		return;
	}

	if (TWeakObjectPtr<UUserWidget>* ExistingPtr = GInventoryScreensByComponent.Find(InventoryComponent))
	{
		if (UUserWidget* Existing = ExistingPtr->Get())
		{
			Existing->RemoveFromParent();
		}
		GInventoryScreensByComponent.Remove(InventoryComponent);
	}
}

void UYIInventoryUIScreenLibrary::CloseAllScreensForComponent(UYIInventoryComponent* InventoryComponent)
{
	CloseInventoryScreenForComponent(InventoryComponent);
}

