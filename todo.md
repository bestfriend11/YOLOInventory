# YOLOInventory – Dungeon Siege-style Inventory Plan

This document captures functional requirements and an implementation plan to achieve a Dungeon Siege 1–style inventory system in the YOLOInventory plugin (UE 5.7.1), plus editor tooling needed for authoring.

Note on DungeonSiege directory: `Plugins/YOLOInventory/DungeonSiege` in this repo contains tools, libs, and assets but not readable gameplay source for the inventory system. The plan below consolidates well-known Dungeon Siege inventory behaviors with our current codebase status.

## 1) High-level goals
- Grid-based container inventory with variable-sized items (1x1, 1x2, 2x3, etc.).
- Item class-driven stacking and uniqueness rules:
  - Unique-per-type (at most one stack in a container).
  - Optional stacking with MaxStackCount.
- Real-time UI updates (editor and runtime) – changes reflected immediately in the preview.
- Rich item presentation:
  - Rarity color coding and rich descriptions.
  - Icon/mesh/fx/sound associations.
- Modular item capabilities via composition of concrete “stack entry nodes” (blueprintable, non-abstract).
- Dungeon Siege–like UX:
  - Drag-drop with valid-position hinting; rotate where allowed.
  - Auto-pack/auto-sort; first-fit placement fallback.
  - Visual stack count; rarity badge; rich tooltip.
  - Container-to-container moves (player ↔ stash/merchant) with rules enforced.

## 2) Data model and assets
- UYIInventoryAsset (Data Asset)
  - DisplayName (FText), Description (FText multi-line)
  - Rarity: EYOLOItemRarity (Common, Uncommon, Rare, Epic, Legendary, Mythic)
  - Icon: TSoftObjectPtr<UTexture2D>
  - Stacking/uniqueness:
    - bUniquePerType (bool), bAllowStacking (bool), MaxStackCount (int32≥1)
  - Default size (FIntPoint Width/Height)
  - Capabilities: array of UYIStackEntry (instanced)
- UYIStackEntry (abstract) and concrete subclasses (BlueprintType, EditInlineNew)
  - UI: UYIUI_NameDesc (Name/Description/Icon, MeshSkeletal, MeshStatic, Effect, DropSound)
  - Ability: UYIAbility_GrantAbility (AbilityClass/Level), UYIAbility_Physics (physics/material)
  - Upgrade/Economy: UYIUpgrade_Path, UYIEconomy_Market, UYIEconomy_DurabilityRules
  - All entries include Rarity override for per-stack coloring if needed
- UYIInventoryBag (container)
  - GridSize (FIntPoint) with MinifyScale (0.1–1.0) and bAllowRotation
  - Items: TArray<FYIBagItem> where FYIBagItem holds Item (soft ptr to UYIInventoryAsset), Pos (FIntPoint), Size (FIntPoint), Count (int32)

## 3) Content Browser integration
- Asset category: “YOLO Inventory” (non-Misc) for all YOLOInventory assets.
  - Ensure AssetTypeActions for UYIInventoryAsset and UYIInventoryBag return GYOLOInventoryAssetCategory.
  - Module startup registers the advanced category and asset actions.

## 4) Placement & rules (DS-style)
- Placement:
  - Item size from asset defaults (with rotation allowed if container supports it).
  - CanPlaceAt(Pos, Size) checks bounds and AABB overlap using EffectiveSize(Size, MinifyScale).
  - First-fit fallback if user-specified cell not available.
- Rotation:
  - If bAllowRotation, toggle W/H on rotate; only commit if CanPlaceAt remains true.
- Stacking & uniqueness:
  - bUniquePerType: only a single stack per item type in a bag.
  - If bAllowStacking && MaxStackCount>1: combine into existing stack up to MaxStackCount.
  - CombineStacks(IndexA, IndexB) merges counts if same item class & stacking enabled.
  - SplitStack(Index, Amount, Position) uses FindFirstFit if preferred Pos fails.

## 5) Editor/UI features
- Inventory Editor (Bag) – tabs: Palette, Grid, Details
  - Grid (SBagEditor): render grid, borders, cells; draw items with rarity color overlay; hover/selection; drag-drop from palette; move/rotate; minify slider; auto-pack.
  - Palette: filterable list of UYIInventoryAsset; add to grid via drag or button.
  - Details: property edit for bag & selected item; reflect changes live.
- Card UI (SYIEntryCard)
  - Header includes rarity swatch, display name, and controls:
    - Pick Icon (Texture2D)
    - Pick Mesh (StaticMesh/SkeletalMesh)
    - Pick FX (UParticleSystem/UNiagaraSystem)
    - Pick Sound (USoundBase)
    - Pick Ability Class (class picker)
  - Rarity dropdown and description preview with rarity-colored text.
- Visual extras
  - Stack count text overlay (bottom-right), rarity border/badge tint.
  - Valid/failing placement hints while dragging (green/red cells).

## 6) Runtime Blueprint API
- UYIInventoryBag functions (BlueprintCallable):
  - AddItem(FYIBagItem&), RemoveItem(Index), MoveItem(Index, NewPos), RotateItem(Index)
  - CombineStacks(IndexA, IndexB), SplitStack(Index, Amount, Position), FindFirstFit(Size, OutPos)
  - FindExistingStackIndex(UYIInventoryAsset*)
  - AutoPack()
- Utility library (BlueprintFunctionLibrary)
  - TransferItemBetweenBags(SourceBag, DestBag, Index/Count)
  - GetFirstEmptyPosForItem(Bag, Item)
  - GetItemTooltipData(Bag, Index) – returns rich text data

## 7) Container interactions
- Bag ↔ Bag transfer logic applies same placement/stacking rules.
- Merchant/stash interactions built atop the transfer functions.
- Optional: cost hooks (stubs) for buy/sell/repair.

## 8) Live refresh & events
- On bag or item property change (editor): post change events.
- SBagEditor subscribes to invalidate/repaint immediately.

## 9) Save/Load & replication (future)
- Ensure FYIBagItem and UYIInventoryBag serialize all fields (including Count, rotated Size, etc.).
- Future: game save support and (optional) replication strategy for multiplayer.

## 10) Performance & UX polish
- O(Items) overlap checks are OK for typical sizes; consider occupancy grid bitset for large bags.
- Auto-pack heuristics: stable row-first pack now; later add better packers.
- Keyboard shortcuts: rotate (R), move with arrows, split (Shift+drag), combine (drag onto same type).

## 11) Testing
- Unit tests: CanPlaceAt, rotation, first-fit, stacking/uniqueness, combine/split.
- Editor tests: drag-drop, minify drop behavior, palette add, details edits live refresh.
- Runtime PIE tests: transfer between two bags; merchant/stash stub.

## 12) Milestones (incremental)
1. Finish SYIEntryCard: rarity dropdown, description preview; live refresh on edits.
2. Grid overlays: stack count label; drag valid/fail cell highlighting.
3. Runtime BP: combine/split/transfer functions; show counts update live.
4. Palette filters: by rarity and by name; search box.
5. Rich tooltips with rarity color markup and capability summaries.
6. Optional: better auto-pack and occupancy grid for large inventories.

## 13) Acceptance criteria
- Content Browser category: all YOLOInventory assets under “YOLO Inventory”.
- Drag-drop obeys stacking/uniqueness and shows counts; rotate works where allowed.
- Editor UI updates immediately on property changes (no refresh action required).
- Blueprint API usable in PIE to move/split/combine items between bags.
- Rich tooltips and rarity visuals present in both editor and runtime.
