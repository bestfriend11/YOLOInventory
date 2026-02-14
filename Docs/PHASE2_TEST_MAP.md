# Phase 2 Test Map (ThirdPerson)

This setup gives you a fast runtime validation map for:
- main inventory bag
- spellbook bag acceptance rules
- drag/drop transfer behavior
- spellbook equip slot -> action bar preset wiring

## Added runtime helper

- Actor: `AYIPhase2TestMapActor`
- Module path: `Plugins/YOLOInventory/Source/YOLOInventory`
- Place this actor in your map and configure it in Details.

## Quick setup steps

1. Open your ThirdPerson map (or create a new one from ThirdPerson template).
2. Place one `YIPhase2TestMapActor` in the level.
3. Configure these fields on the actor:
   - `SpellbookAcceptedItemTypeTag` = your spell root type (example: `Item.Spell`)
   - `SpellbookEquipSlotTag` = equipment slot for spellbook (example: `Equip.Slot.Spellbook.Primary`)
   - Optional: `MainBagRoleTag` / `SpellbookBagRoleTag` (recommended for ID-based UI binding migration)
   - `InventoryScreenClass` = your `UInventoryScreenWidget` blueprint class
   - Optional:
     - `MainBagTemplate`
     - `SpellbookBagTemplate`
     - `StarterItems`
4. Press Play (Listen Server + one client for multiplayer checks).

### Optional UMG binding (Phase 2)

- In your `UInventoryScreenWidget` blueprint, add a second `UInventoryGridWidget` named `SpellbookGrid` (BindWidgetOptional).
- The screen now binds:
  - `Grid` -> active inventory bag context
  - `SpellbookGrid` -> active spellbook bag context
- This uses bag identity contexts (`ActiveBagId` / `ActiveSpellbookBagId`) instead of bag-array index assumptions.

## What it does at runtime

On authority (server), the actor:
- ensures `UYIInventoryComponent`, `UYIEquipmentComponent`, and `UYIActionBarComponent` exist on the target pawn
- creates runtime bags (or clones templates)
- enforces spellbook bag acceptance (`bEnforceAcceptanceRules`, `AllowedItemTypes`)
- seeds starter items into main/spellbook bags
- opens main bag
- applies spellbook action preset via `UYIInventoryGameplaySetupLibrary::ApplySpellbookActionPreset`
- runs validation and logs diagnostics

On local player, it opens the inventory screen automatically (if enabled).

## Validation checklist

- Drag non-spell item to spellbook bag: blocked.
- Drag spell item to spellbook bag: accepted.
- Drag spell onto occupied spell cell: swap/replace without item loss.
- Verify on-screen and log messages from setup actor for diagnostics.

## Useful actor toggles

- `bResetBagsBeforeSetup`: clean deterministic startup each PIE run.
- `bSetupOnBeginPlay`: auto-bootstrap.
- `bOpenInventoryScreenOnBeginPlay`: immediate UI feedback.
- `RunSetupNow` (CallInEditor): manual retry trigger.
- `CreatePresetBlueprintAssetFromCurrentSettings` (CallInEditor): generates a reusable blueprint preset actor in your chosen `/Game/...` folder.
