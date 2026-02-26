#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryDragOverlayUserWidget.generated.h"

/**
 * Inventory-wide drag overlay that renders a single global drag ghost at the cursor
 * when an inventory drag is active. Per-grid ghosts should be disabled by enabling
 * bUseGlobalDragGhost on the grids.
 */
UCLASS()
class YOLOINVENTORYGRID_API UInventoryDragOverlayUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UInventoryDragOverlayUserWidget(const FObjectInitializer& OI);

	// Desired visual size of the ghost if no icon/size is available
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Visuals")
	FVector2D FallbackGhostSize = FVector2D(64.f, 64.f);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:
	// Optional references to grids to enable footprint highlighting under cursor
	UPROPERTY(BlueprintReadWrite, Category="Inventory|Overlay")
	TWeakObjectPtr<class UInventoryGridWidget> LeftGrid;
	UPROPERTY(BlueprintReadWrite, Category="Inventory|Overlay")
	TWeakObjectPtr<class UInventoryGridWidget> RightGrid;

private:
	mutable FVector2D CachedCursorSS = FVector2D::ZeroVector;
	mutable FVector2D CachedCursorLocal = FVector2D::ZeroVector;
	mutable bool bShouldDraw = false;
};
