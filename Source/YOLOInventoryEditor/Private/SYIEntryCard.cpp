#include "SYIEntryCard.h"
#include "YIStackEntry.h"
#include "YIStackEntry_Examples.h"
#include "YIInventoryTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"
#include "Input/Reply.h"
#include "Framework/Application/SlateApplication.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Kismet2/SClassPickerDialog.h"
#include "ClassViewerModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

void SYIEntryCard::Construct(const FArguments& InArgs)
{
	Index = InArgs._Index;
	Entry = InArgs._Entry;
	StackName = InArgs._StackName;
	OnUp = InArgs._OnUp; OnDown = InArgs._OnDown; OnDup = InArgs._OnDup; OnDel = InArgs._OnDel; OnSelected = InArgs._OnSelected;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(8,6))
		.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[
				SNew(SBox).WidthOverride(24).HeightOverride(24)
				[
					SNew(SImage)
						.ColorAndOpacity_Lambda([this]() { return Entry ? YI_GetRarityColor(Entry->Rarity) : FLinearColor::White; })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(8,0,0,0))[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[
					SNew(STextBlock)
					.Text_Lambda([this]() { return Entry ? (Entry->DisplayName.IsEmpty() ? FText::FromString(TEXT("Entry")) : Entry->DisplayName) : FText::FromString(TEXT("Entry")); })
				]
				+ SVerticalBox::Slot().AutoHeight()[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
					.ColorAndOpacity_Lambda([this]() { return Entry ? YI_GetRarityColor(Entry->Rarity) : FLinearColor::White; })
					.Text_Lambda([this]() {
						if (auto UI = Cast<UYIUI_NameDesc>(Entry)) { return UI->Description; }
						return FText();
					})
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4,0,0,0))[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.Text(NSLOCTEXT("YOLOInventory","Rarity","Rarity"))
				.OnClicked_Lambda([this]() -> FReply {
					if (Entry)
					{
						FMenuBuilder Menu(true, nullptr);
						auto AddR = [this,&Menu](EYOLOItemRarity R, const FText& Label)
						{
							Menu.AddMenuEntry(Label, FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this,R](){ if (Entry) { Entry->Modify(); Entry->Rarity = R; }})));
						};
						AddR(EYOLOItemRarity::Common,    NSLOCTEXT("YOLOInventory","R_Common","Common"));
						AddR(EYOLOItemRarity::Uncommon,  NSLOCTEXT("YOLOInventory","R_Uncommon","Uncommon"));
						AddR(EYOLOItemRarity::Rare,      NSLOCTEXT("YOLOInventory","R_Rare","Rare"));
						AddR(EYOLOItemRarity::Epic,      NSLOCTEXT("YOLOInventory","R_Epic","Epic"));
						AddR(EYOLOItemRarity::Legendary, NSLOCTEXT("YOLOInventory","R_Legendary","Legendary"));
						AddR(EYOLOItemRarity::Mythic,    NSLOCTEXT("YOLOInventory","R_Mythic","Mythic"));
						FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Menu.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4,0,0,0))[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.Text(NSLOCTEXT("YOLOInventory","PickIcon","Pick Icon"))
				.OnClicked_Lambda([this]() -> FReply {
					if (Entry)
					{
						// Open asset picker for icon
						TSharedRef<SWidget> Picker = PropertyCustomizationHelpers::MakeAssetPickerAnchorButton(
							FOnGetAllowedClasses::CreateLambda([](TArray<const UClass*>& Out){ Out = { UTexture2D::StaticClass() }; }),
							FOnAssetSelected::CreateLambda([this](const FAssetData& AD)
							{
								if (auto UI = Cast<UYIUI_NameDesc>(Entry)) { UI->Modify(); UI->Icon = TSoftObjectPtr<UTexture2D>(AD.ToSoftObjectPath()); }
							})
						);
						FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Picker, FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4,0,0,0))[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.Text(NSLOCTEXT("YOLOInventory","PickMesh","Pick Mesh"))
				.OnClicked_Lambda([this]() -> FReply {
					if (Entry)
					{
						TSharedRef<SWidget> Picker = PropertyCustomizationHelpers::MakeAssetPickerAnchorButton(
							FOnGetAllowedClasses::CreateLambda([](TArray<const UClass*>& Out){ Out = { UStaticMesh::StaticClass(), USkeletalMesh::StaticClass() }; }),
							FOnAssetSelected::CreateLambda([this](const FAssetData& AD)
							{
								if (auto UI = Cast<UYIUI_NameDesc>(Entry)) { UI->Modify(); if (AD.GetClass()->IsChildOf(USkeletalMesh::StaticClass())) UI->MeshSkeletal = TSoftObjectPtr<USkeletalMesh>(AD.ToSoftObjectPath()); else if (AD.GetClass()->IsChildOf(UStaticMesh::StaticClass())) UI->MeshStatic = TSoftObjectPtr<UStaticMesh>(AD.ToSoftObjectPath()); }
							})
						);
						FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Picker, FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4,0,0,0))[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.Text(NSLOCTEXT("YOLOInventory","PickFX","Pick FX"))
				.OnClicked_Lambda([this]() -> FReply {
					if (Entry)
					{
						TSharedRef<SWidget> Picker = PropertyCustomizationHelpers::MakeAssetPickerAnchorButton(
							FOnGetAllowedClasses::CreateLambda([](TArray<const UClass*>& Out){ Out = { UParticleSystem::StaticClass(), UNiagaraSystem::StaticClass() }; }),
							FOnAssetSelected::CreateLambda([this](const FAssetData& AD)
							{
								if (auto UI = Cast<UYIUI_NameDesc>(Entry)) { UI->Modify(); UI->Effect = TSoftObjectPtr<UFXSystemAsset>(AD.ToSoftObjectPath()); }
							})
						);
						FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Picker, FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4,0,0,0))[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.Text(NSLOCTEXT("YOLOInventory","PickSound","Pick Sound"))
				.OnClicked_Lambda([this]() -> FReply {
					if (Entry)
					{
						TSharedRef<SWidget> Picker = PropertyCustomizationHelpers::MakeAssetPickerAnchorButton(
							FOnGetAllowedClasses::CreateLambda([](TArray<const UClass*>& Out){ Out = { USoundBase::StaticClass() }; }),
							FOnAssetSelected::CreateLambda([this](const FAssetData& AD)
							{
								if (auto UI = Cast<UYIUI_NameDesc>(Entry)) { UI->Modify(); UI->DropSound = TSoftObjectPtr<USoundBase>(AD.ToSoftObjectPath()); }
							})
						);
						FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Picker, FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4,0,0,0))[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.Text(NSLOCTEXT("YOLOInventory","PickAbility","Pick Ability Class"))
				.OnClicked_Lambda([this]() -> FReply {
					if (Entry)
					{
						UClass* ChosenClass = nullptr;
						FClassViewerInitializationOptions Options; Options.bShowUnloadedBlueprints = true; Options.Mode = EClassViewerMode::ClassPicker;
						SClassPickerDialog::PickClass(NSLOCTEXT("YOLOInventory","PickAbilityClass","Pick Ability Class"), Options, ChosenClass, UObject::StaticClass());
						if (ChosenClass) { if (auto A = Cast<UYIAbility_GrantAbility>(Entry)) { A->Modify(); A->AbilityClass = ChosenClass; } }
					}
					return FReply::Handled();
				})
			]
		]
	];
}

FReply SYIEntryCard::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (OnSelected.IsBound())
		{
			OnSelected.Execute(Entry);
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}
