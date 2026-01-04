# YOLOInventory – Session TODO (Progress + Next Steps)

Date: [fill-in]

## What we completed this session

Stabilization and UX (Phase 0 – Step 1: Global drag overlay)
- Added per-grid ghost suppression flag
  - UInventoryGridWidget: `bool bUseGlobalDragGhost` (+ `SetUseGlobalDragGhost`) to disable per-grid ghost drawing
  - Propagated to SInventoryGridWidget and respected in paint path
- Implemented a global drag overlay widget
  - `UInventoryDragOverlayUserWidget` (UserWidget) renders the ghost once for the whole screen
  - Fixed cursor offset in windowed mode using overlay geometry `AbsoluteToLocal`
  - Draws a placement footprint highlight (green/red) on whichever grid the cursor is over
- Global registry of live grids
  - `UInventoryGridWidget` registers in `OnWidgetRebuilt` and unregisters in `BeginDestroy`
  - `static TSet<TWeakObjectPtr<UInventoryGridWidget>> GRegisteredGrids` + `ForEachRegisteredGrid` helper for overlay discovery
- Trading screen integration
  - If a drag overlay widget is present on the Trading screen, both grids switch to global ghost mode
  - Overlay queries all registered grids; no manual Left/Right binding needed
- Lingering ghost fixes (both grids)
  - Disables ghost when cursor leaves grid bounds; added safety in Tick to enforce the rule

Related improvements from earlier steps
- Same-bag drop behavior: “displace and continue dragging (unattached)”
  - Dropping onto an occupied cell removes the victim and continues dragging the victim unattached (no swap-backfill)
- Respect highlighted cell placement strictly (no fallback to first-fit/top-left when dropping on a valid highlight)
- Trading screen core
  - `UTradingScreenWidget` with two grids and tooltips, UILayered integration
  - `SetBag` on grid to rebind bags safely; `RefreshBoundTooltip` in tooltip flow

## Files touched (high-level)
- Public
  - Plugins/YOLOInventory/Source/YOLOInventory/Public/InventoryGridWidget.h
  - Plugins/YOLOInventory/Source/YOLOInventory/Public/SInventoryGridWidget.h
  - Plugins/YOLOInventory/Source/YOLOInventory/Public/InventoryDragOverlayUserWidget.h
  - Plugins/YOLOInventory/Source/YOLOInventory/Public/TradingScreenWidget.h
- Private
  - Plugins/YOLOInventory/Source/YOLOInventory/Private/InventoryGridWidget.cpp
  - Plugins/YOLOInventory/Source/YOLOInventory/Private/SInventoryGridWidget.cpp
  - Plugins/YOLOInventory/Source/YOLOInventory/Private/InventoryDragOverlayUserWidget.cpp
  - Plugins/YOLOInventory/Source/YOLOInventory/Private/TradingScreenWidget.cpp

## Quick test notes
- Windowed vs Fullscreen: overlay ghost now aligns with cursor in both modes
- Cross-grid: per-grid ghosts are suppressed when overlay is active; overlay renders once
- Hover: footprint highlight appears only for the grid under the cursor and respects placement validity

## Proposed next session – targeted tasks
1) Overlay visuals – item icon & footprint sizing
   - Render the dragged item’s actual icon/brush in overlay
   - Size icon to the footprint using hovered grid’s cell size
   - Optional: alpha pulse or outline to improve visibility

2) Overlay lifecycle – show/hide signals
   - Add drag-start/end notifications from grid (or central drag state) to explicitly show/hide overlay
   - Trading screen: toggle overlay visibility when drag begins/ends for cleaner UX

3) Cross-bag behavior option
   - Add `bCrossBagDisplaceVictimUnattached` to match same-bag rules for cross-bag drops when enabled
   - Keep existing atomic swap as default if option is disabled

4) Trading sample UMG and BP
   - Provide a ready-to-open sample Trading screen (two grids, tooltips, overlay on top)
   - Blueprint to create demo bags and open the screen for easy validation

5) Tooltip flow option (if desired)
   - Add `bHoverDrivesTooltip` to refresh tooltip on hover when no selection is present

6) Code cleanup & PR
   - Guard new feature flags with sensible defaults
   - Prepare a PR with notes, and update `todo.md`/`system_goal.md` links

## Open questions
- Should cross-bag default behavior switch to “displace unattached” for consistency, or remain atomic swap by default?
- Do we want to support a global overlay outside the Trading screen (e.g., general inventory UI)? If so, ship an overlay ScreenWidget in the plugin.
