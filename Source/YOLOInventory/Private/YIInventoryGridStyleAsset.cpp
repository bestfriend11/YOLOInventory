#include "YIInventoryGridStyleAsset.h"

UYIInventoryGridStyleAsset::UYIInventoryGridStyleAsset()
{
	HoveredCellOverlay.bEnabled = true;
	HoveredCellOverlay.Tint = FLinearColor(0.1f, 0.6f, 1.f, 0.08f);

	HoveredItemOverlay.bEnabled = true;
	HoveredItemOverlay.Tint = FLinearColor(0.1f, 0.8f, 0.2f, 0.12f);

	SelectedCellOverlay.bEnabled = false;
	SelectedCellOutlineColor = FLinearColor(0.2f, 0.6f, 1.f, 0.9f);
	SelectedCellOutlineThickness = 2.5f;

	LockedItemOverlay.bEnabled = true;
	LockedItemOverlay.Tint = FLinearColor(0.16f, 0.42f, 0.9f, 0.24f);

	GhostPlacementValidOverlay.bEnabled = true;
	GhostPlacementValidOverlay.Tint = FLinearColor(0.2f, 0.8f, 0.2f, 0.18f);

	GhostPlacementInvalidOverlay.bEnabled = true;
	GhostPlacementInvalidOverlay.Tint = FLinearColor(0.8f, 0.2f, 0.2f, 0.18f);
}

