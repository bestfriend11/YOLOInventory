# Runtime Quickstart (From Zero to Playable)

This is the fastest path to a working in-game inventory with minimal wiring.

## 1) Pawn setup

- Use your game pawn (for example `BP_ThirdPersonCharacter`).
- You do **not** need to pre-add inventory bags manually.

## 2) Server bootstrap (one node)

In the pawn Blueprint `BeginPlay`:

- Add authority gate (`Has Authority`).
- Call `Quick Start Pawn Inventory` (`UYIInventoryGameplaySetupLibrary`).
- `bOpenInventoryScreen = false` on server.

This will:
- ensure/create `UYIInventoryComponent`, `UYIEquipmentComponent`, `UYIActionBarComponent`
- ensure at least one default bag exists and is opened
- validate setup and return summary/warnings/errors

## 3) Client UI open (one node)

On local player (same `BeginPlay`, non-authority branch) or on your inventory input action:

- Call `Quick Start Pawn Inventory` with `bOpenInventoryScreen = true`.

If no `InventoryScreenClass` is assigned, the plugin now falls back to `UInventoryScreenWidget`.
If your screen Blueprint is missing `Grid`/`Tooltip`, the widget creates a minimal default layout automatically.

## 4) Optional custom screen Blueprint

For a polished UI, create `WBP_InventoryScreen` derived from `UInventoryScreenWidget` and add:

- `Grid` (`UInventoryGridWidget`)
- `Tooltip` (`UInventoryTooltipView`)
- optional `SpellbookGrid`
- optional `EquipmentSlotsPanel` or `EquipmentSlotsCanvasPanel`

Assign this class to `UYIInventoryComponent.InventoryScreenClass`.

## 5) Equipment schema (recommended)

- Create an `Equipment Schema` asset (Content Browser -> YOLO Inventory).
- Define slot tags and accepted item tags there.
- On your pawn `UYIEquipmentComponent`, set `EquipmentSchemaAsset`.
- Keep UI layout in UMG (`UInventoryEquipmentSlotWidget` + `SlotTag`), not in schema.

## 6) Troubleshoot quickly

- Read `OutResult.Summary`, `BlockingIssues`, `Warnings` from `Quick Start Pawn Inventory`.
- If UI does not open, call it from the local owning client and verify pawn ownership/controller.
