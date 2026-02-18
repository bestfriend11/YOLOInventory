#include "YIAutoPickupDropActorDetails.h"

#include "YIAutoPickupDropActor.h"
#include "YIItemDefinition.h"
#include "AssetRegistry/AssetData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FYIAutoPickupDropActorDetails::MakeInstance()
{
	return MakeShared<FYIAutoPickupDropActorDetails>();
}

void FYIAutoPickupDropActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	for (const TWeakObjectPtr<UObject>& Obj : Objects)
	{
		if (AYIAutoPickupDropActor* DropActor = Cast<AYIAutoPickupDropActor>(Obj.Get()))
		{
			EditingActor = DropActor;
			break;
		}
	}

	ItemDefinitionHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(AYIAutoPickupDropActor, ItemDefinition), AYIAutoPickupDropActor::StaticClass());
	if (!ItemDefinitionHandle.IsValid())
	{
		return;
	}

	DetailBuilder.HideProperty(ItemDefinitionHandle);

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("YOLOInventory|Pickup"));
	Category.AddCustomRow(FText::FromString(TEXT("Item Definition")))
	.NameContent()
	[
		ItemDefinitionHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(420.f)
	.MaxDesiredWidth(0.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SObjectPropertyEntryBox)
			.PropertyHandle(ItemDefinitionHandle)
			.AllowedClass(UYIItemDefinition::StaticClass())
			.DisplayThumbnail(true)
			.OnObjectChanged(this, &FYIAutoPickupDropActorDetails::OnItemDefinitionChanged)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(8.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(64.f)
					.HeightOverride(64.f)
					[
						SNew(SImage).Image(this, &FYIAutoPickupDropActorDetails::GetPreviewBrush)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(this, &FYIAutoPickupDropActorDetails::GetTitleText)
						.Font(FAppStyle::Get().GetFontStyle("PropertyWindow.BoldFont"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(this, &FYIAutoPickupDropActorDetails::GetSubtitleText)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f)))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(this, &FYIAutoPickupDropActorDetails::GetDescriptionText)
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.86f, 0.86f)))
					]
				]
			]
		]
	];
}

UYIItemDefinition* FYIAutoPickupDropActorDetails::ResolveDefinition() const
{
	AYIAutoPickupDropActor* DropActor = EditingActor.Get();
	if (!DropActor)
	{
		return nullptr;
	}
	return DropActor->ItemDefinition.IsValid()
		? DropActor->ItemDefinition.Get()
		: DropActor->ItemDefinition.LoadSynchronous();
}

const FSlateBrush* FYIAutoPickupDropActorDetails::GetPreviewBrush() const
{
	if (UYIItemDefinition* Def = ResolveDefinition())
	{
		if (UTexture2D* Icon = Def->Icon.IsValid() ? Def->Icon.Get() : Def->Icon.LoadSynchronous())
		{
			PreviewBrush.SetResourceObject(Icon);
			PreviewBrush.ImageSize = FVector2D(Icon->GetSizeX(), Icon->GetSizeY());
			PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
			return &PreviewBrush;
		}
	}

	return FAppStyle::Get().GetBrush("ClassIcon.Texture2D");
}

FText FYIAutoPickupDropActorDetails::GetTitleText() const
{
	if (UYIItemDefinition* Def = ResolveDefinition())
	{
		if (!Def->DisplayName.IsEmpty())
		{
			return Def->DisplayName;
		}
		return FText::FromString(Def->GetName());
	}
	return FText::FromString(TEXT("No item selected"));
}

FText FYIAutoPickupDropActorDetails::GetSubtitleText() const
{
	if (UYIItemDefinition* Def = ResolveDefinition())
	{
		return FText::FromString(FString::Printf(
			TEXT("Code: %lld   Template: %s   Size: %dx%d"),
			static_cast<long long>(Def->UniqueCode),
			Def->TemplateId.IsEmpty() ? TEXT("-") : *Def->TemplateId,
			Def->DefaultSize.X,
			Def->DefaultSize.Y));
	}
	return FText::FromString(TEXT("Pick an Item Definition asset to drive this drop"));
}

FText FYIAutoPickupDropActorDetails::GetDescriptionText() const
{
	if (UYIItemDefinition* Def = ResolveDefinition())
	{
		if (!Def->Description.IsEmpty())
		{
			return Def->Description;
		}
		return FText::FromString(TEXT("No description on this item."));
	}
	return FText::FromString(TEXT("This chooser shows icon + gameplay fields so designers can pick items by intent, not just asset name."));
}

void FYIAutoPickupDropActorDetails::OnItemDefinitionChanged(const FAssetData& AssetData)
{
	AYIAutoPickupDropActor* DropActor = EditingActor.Get();
	if (!DropActor || !AssetData.IsValid())
	{
		return;
	}

	if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(AssetData.GetAsset()))
	{
		DropActor->ItemDefinition = Def;
	}
}
