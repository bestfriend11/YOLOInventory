#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class AYIAutoPickupDropActor;
class UYIItemDefinition;
struct FAssetData;

class FYIAutoPickupDropActorDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	UYIItemDefinition* ResolveDefinition() const;
	const FSlateBrush* GetPreviewBrush() const;
	FText GetTitleText() const;
	FText GetSubtitleText() const;
	FText GetDescriptionText() const;

	void OnItemDefinitionChanged(const FAssetData& AssetData);

private:
	TWeakObjectPtr<AYIAutoPickupDropActor> EditingActor;
	TSharedPtr<IPropertyHandle> ItemDefinitionHandle;
	mutable FSlateBrush PreviewBrush;
};
