# Debug notes: InventoryDragOverlayUserWidget overlay highlight

This document describes the debug instrumentation added to help diagnose incorrect highlight/footprint rendering during inventory drag operations.

## What changed

File: `Plugins/YOLOInventory/Source/YOLOInventory/Private/InventoryDragOverlayUserWidget.cpp`
- Method: `UInventoryDragOverlayUserWidget::NativePaint(...)`
- Added comprehensive per-frame on-screen debug output using `GEngine->AddOnScreenDebugMessage` with a fixed message key so it updates in place (no spam).
- Kept all existing drawing logic intact; the change is purely additive for diagnostics.

## What is logged
The debug block prints the following each frame:

General overlay info
- `bShouldDraw`: whether the overlay should draw (based on active drag state in `NativeTick`).
- `CachedCursorSS`: current absolute/screen-space cursor used by the overlay.
- `Allotted LocalSize`: the overlay widget’s local size.
- `Local(cursor in overlay)`: cursor converted to overlay-local via `AllottedGeometry.AbsoluteToLocal`.
- `GhostSize` and `P(top-left)`: the debug ghost’s size and top-left position.
- `LayerId(start)` and `LayerId(after ghost)`: the layer identifiers across draw calls.

Hovered grid selection
- For each registered grid: `Grid <Name> AbsRect: (L,T)-(R,B) hit:Y|N` using `GetLayoutBoundingRect()` and containment against `CachedCursorSS`.
- `HoveredGrid`: the grid chosen as hovered (first containment match), or `<none>`.

Footprint details (for hovered grid)
- `GridLocalFromScreen`: cursor mapped into the grid’s local space.
- `GridLocalSize`: grid widget’s local size.
- `Inside`: whether the grid-local cursor is within bounds.
- `DragItem.Size`: dragged item’s size in cells.
- `Foot(effective)`: size after `Bag->GetEffectiveSize(DragItem.Size)` (accounts for rotations etc. if applicable).
- `CellPx`: cell size in pixels from `Grid->GetCellPixelSize()`.
- `CursorCellF`: cursor expressed in fractional cell space.
- `DesiredTopLeft(clamped)`: proposed top-left cell for footprint after clamping to grid.
- `GridSize`: bag/grid size in cells.
- `bCanPlace`: result of `Bag->CanPlaceAt(DesiredTopLeft, DragItem.Size)`.
- `FootP_Grid`, `FootS_Grid`, `FootP_Abs`, `FootP_Overlay`: footprint position/size in grid-local, absolute, and overlay-local spaces.
- `LayerId(after footprint)`: layer identifier after footprint draw.

## How to use
1. Run the editor and start an inventory drag.
2. Observe the yellow on-screen debug text (updates every frame). It should help identify whether cursor space conversions, hover picking, or footprint math are off.
3. If you want a new line every frame (instead of updating the same line), change the message key from the fixed value (`0x99110001`) to `INDEX_NONE`.

## Performance impact
- The overlay widget is marked volatile and is cheap to repaint.
- The debug string builds each frame during a drag; acceptable for diagnostic sessions but consider gating when not needed.

## Toggling suggestions (optional follow-ups)
- Introduce a console variable (e.g., `yolo.InventoryOverlayDebug 0/1`) and guard the debug string generation and `AddOnScreenDebugMessage` calls.
- Add a `bEnableOverlayDebug` UPROPERTY to toggle in editor.

## Why this helps the highlight bug
The highlight often goes wrong due to a mismatch between coordinate spaces (screen/absolute vs. local), DPI scaling, window offsets, or clamping logic in cell space. This instrumentation prints values at each step so you can verify:
- The hovered grid selection is correct and uses absolute rect containment.
- The cursor-to-local conversions (`AbsoluteToLocal`) are consistent between overlay and grid widgets.
- The footprint’s proposed top-left, clamping, and placement check line up with visual rendering.

## Reverting
- All changes are confined to `NativePaint`. Removing the added debug string building and `AddOnScreenDebugMessage` calls restores prior behavior.
