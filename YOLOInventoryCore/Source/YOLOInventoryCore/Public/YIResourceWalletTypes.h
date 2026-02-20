#pragma once

#include "CoreMinimal.h"
#include "YIResourceWalletTypes.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIResourceEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	int64 Amount = 0;
};

/** Generic resource/currency wallet keyed by designer-defined names (Gold, Silver, Iron, Oil, Food, etc.). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIResourceWallet
{
	GENERATED_BODY()

	/** Resource entries; replicated as array for net safety. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	TArray<FYIResourceEntry> Entries;

	/** Convenience getter. Returns 0 if missing. */
	int64 Get(const FName& Name) const
	{
		if (const FYIResourceEntry* Ptr = Entries.FindByPredicate([&](const FYIResourceEntry& E){ return E.Name == Name; }))
		{
			return Ptr->Amount;
		}
		return 0;
	}

	/** Add (or subtract) a delta. Negative is spend. Returns new value. */
	int64 Add(const FName& Name, int64 Delta)
	{
		if (FYIResourceEntry* Ptr = Entries.FindByPredicate([&](const FYIResourceEntry& E){ return E.Name == Name; }))
		{
			Ptr->Amount = FMath::Max<int64>(0, Ptr->Amount + Delta);
			return Ptr->Amount;
		}
		FYIResourceEntry NewEntry;
		NewEntry.Name = Name;
		NewEntry.Amount = FMath::Max<int64>(0, Delta);
		Entries.Add(NewEntry);
		return NewEntry.Amount;
	}

	/** Try to consume Amount; returns false if insufficient. */
	bool Consume(const FName& Name, int64 Amount)
	{
		if (Amount <= 0) return true;
		if (FYIResourceEntry* Ptr = Entries.FindByPredicate([&](const FYIResourceEntry& E){ return E.Name == Name; }))
		{
			if (Ptr->Amount < Amount) return false;
			Ptr->Amount -= Amount;
			return true;
		}
		return false;
	}
};

