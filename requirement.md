# YOLOInventory – DS1-style Inventory Requirements

Status: Draft (to be refined as we ingest DS1 code and iterate)
Owner: YOLOInventory Team

## Purpose
Design and implement a Dungeon Siege 1–like inventory and item authoring pipeline in Unreal Engine (5.7), with a modern modular item system and rich editor tooling.

## Sources to analyze (pending mirror into repo)
- D:\aa\MyProject4-571\Plugins\YOLOInventory\DungeonSiege\lib projects
- D:\aa\MyProject4-571\Plugins\YOLOInventory\DungeonSiege\projects
Action: Mirror these directories under Plugins/YOLOInventory/DungeonSiege/ in this repo so we can index and align behavior precisely.

## High-level goals
- DS1-like grid inventory with variable-sized items and rotation where allowed.
- Stacking and uniqueness rules per item type.
- Modular, capability-based item assets (composition over inheritance).
- Robust editor tooling: Item editor (capability cards) and Bag editor (grid with drag/rotate/stack visuals).
- Runtime Blueprint/C++ API for add/move/rotate/split/combine/transfer.
- Visual polish: rarity borders, stack counts, placement hints, tooltips.
- Save/Load support; future replication.

## Core data model (initial proposal)
- UYIInventoryAsset
  - DisplayName, Description, Icon, Rarity
  - DefaultSize (Width x Height), bAllowStacking, MaxStackCount, bUniquePerType
  - Capabilities: array of UYIStackEntry (instanced, BlueprintType, EditInlineNew)
- Capability families (examples):
  - UI_Presentation (meshes, FX, sounds, rarity overrides)
  - Economy (buy/sell/repair)
  - Durability (current/max, break rules)
  - Equip (slots, requirements, weight)
  - GAS (granted ability class + tags)
  - Crafting (recipes, salvage, upgrades)
  - UseAction (consumable/world use)
- UYIInventoryBag
  - GridSize, bAllowRotation, MinifyScale (editor)
  - Items: TArray<FYIBagItem>
- FYIBagItem
  - Item (soft asset), Pos, Size (post-rotation), Count, bRotated, bLocked, GUID

## Rules and algorithms (to match DS1 semantics)
- Placement priority: user-chosen cell; fallback to first-fit (row-first scan).
- Rotation: allowed per-container; commit only if CanPlaceAt still true.
- Stacking/Uniqueness:
  - Unique-per-type: at most one stack of that item class in a bag.
  - Stacking: fill existing stacks up to MaxStackCount before creating new stack.
- Auto-pack: stable row-first pack (MVP); later, improved heuristics optional.

## Editor tooling
- Asset Category: YOLO Inventory (register on module startup).
- Item Editor
  - Capability palette + stack cards with details panel
  - Live previews: icon, rarity/tint, tooltip
  - Validation: missing icon, conflicting equip rules, invalid stacking settings, etc.
- Bag Editor
  - Palette of items; grid view (drag/drop/move/rotate/split/combine)
  - Placement hints, stack count overlays, rarity borders
  - AutoPack button, zoom/pan, minify slider

## Runtime UI and API
- UMG WBP_BagGrid, re-usable for player/stash/merchant
- BlueprintCallable on UYIInventoryBag:
  - AddItem, RemoveItem, MoveItem, RotateItem
  - CombineStacks, SplitStack, FindFirstFit, FindExistingStackIndex, AutoPack
- Blueprint library:
  - TransferItemBetweenBags, GetFirstEmptyPosForItem, GetItemTooltipData

## Persistence & networking
- Serialize FYIBagItem fully (asset path, Pos/Size/Count/flags/GUID)
- Save/Load on UYIInventoryBag
- Future: replication and authoritative server moves

## DS1 alignment tasks (pending DS1 code analysis)
- Confirm exact first-fit placement order and rotation behavior
- Confirm stacking/uniqueness edge-cases (e.g., auto-stack on pickup, merchant constraints)
- Confirm auto-pack algorithm nuances (stability, tie-breaking)
- Confirm item size mapping and rotation limitations by category

## System scope and responsibilities

This plugin will provide core systems and editors for:
- Items and Inventory (grid containers, stacks, placements) [MVP]
- Equipment system (equip slots, requirements, effects) [Phase 2]
- Shops/Merchants (buy/sell/repair stubs) [Phase 2]
- Spellbooks (learned spells as items, usage/integration) [Phase 2]

Non-goals: Full game economy logic, full ability systems, or UI themes. The plugin exposes events/APIs for game code to implement policies.

## Inventory system requirements (DS1-like)
- Containers (bags, stash, merchant) are grid boxes with:
  - GridSize (rows, columns), CellSize (px), Scaling.
  - Flags: IsStore, RequestPickup, ContinuePickup, AutoTransfer, AutoPlaceOnly, LocalPlace, DeactivateOverlapped, ConsumesItems, AutoPlaceNotify, LastSort.
- Placement flow
  - Drag an active item over a container → nearest cell origin chosen.
  - Bounds/footprint validation across W×H from item Rows/Columns.
  - If all cells empty → place item.
  - If cells belong to a single different item → displace it (pick up old first) then place new.
  - If multiple different items under footprint → invalid.
  - Optional: first-fit auto-place scan when explicit position fails.
- Stacking
  - Items may stack up to MaxStackCount; display count overlay if >1.
  - Combine when placing same item into existing stack if permitted; split by UX gesture or API.
  - Uniqueness: optional single-stack-per-type per container.
- Sorting/Auto-pack
  - Row-first first-fit; forward/reverse order; stability preferred. Advanced packers optional.
- Visuals
  - Rollover cell tint for the current footprint; stack number bottom-right; per-item highlight.
- Serialization
  - Persist item asset ref, position, size (rotation), count, flags.

## Item model requirements

Stack-based authoring model
- Stacks are authoring groups designers compose per item using premade, blueprintable templates. Each stack focuses on one aspect (visuals, requirements, audio/VFX, core stats, abilities, etc.). Designers add entries one-by-one to a stack to build up the item.
- Runtime: these stacks instantiate into per-item runtime state (so items can vary instance-by-instance if needed).

Recommended stack categories (editor tabs or grouped sections)
- Presentation stack
  - Name, Rarity, Icon, Mesh (Static/Skeletal), Material override, Secondary overlay (texture, edge, percent), Color accents.
- Core stats stack
  - Defense/Attack/Armor class, Weight, Value, Durability, Level tier.
- Requirements stack
  - Required attributes (e.g., Strength 55), Required level, Required tags/classes.
- Audio/VFX stack
  - DropSound, UseSound, EquipSound, HitSound; FX on equip/use/hit.
- Size/Placement stack
  - DefaultSize (Rows, Columns), Rotation allowed, ItemType/SlotType.
- Abilities stack (the “ability per stack” authoring)
  - A list of UYIAbilityAsset references (see below) that grant modifiers, procs, or gameplay abilities with friendly text.
- Economy stack
  - Sell value, Buy price, Repair cost modifiers, Vendor tags.
- Tags/Metadata stack
  - Gameplay Tags, rarity overrides, categorization, search filters.

Ability assets (wrapped, designer-friendly)
- UYIAbilityAsset (base UObject, BlueprintType)
  - DisplayName, Description (rich text), Icon, Rarity hint (optional), GameplayTags, StackingBehavior (Stackable, UniquePerItem, UniquePerCharacter), ConflictRules.
  - Tooltip builder: GenerateTooltipLines(const FYIAbilityContext&).
  - Activation hooks: OnEquip/OnUnequip, OnUse, OnTick, OnEvent (e.g., OnDamageTaken, OnHitDealt).
- Subclasses
  - UYIAbility_Modifier: e.g., +5 Strength, +15% Defense. Fields: Attribute/Tag, Additive/Multiplicative, Value, ScalingByLevel.
  - UYIAbility_Proc: e.g., 18% chance to cast Nova Strike on taking damage. Fields: Event (OnDamageTaken), Chance, Cooldown, Effect (could reference UYIEffectAsset or UYIAbility_GASWrapper), Conditions.
  - UYIAbility_GASWrapper: wraps a Gameplay Ability class with friendly metadata; exposes tags, level, cooldown, and description templates.
  - UYIAbility_RawEffect: simple scripted effect with blueprint-executed logic for teams without GAS.

Runtime ability execution
- Event bus: Inventory component raises events (OnEquip, OnUnequip, OnUse, OnDamageTaken, OnAttack, etc.). Ability assets subscribe via capability lifecycle or a dispatcher.
- Stacking rules enforced by UYIAbilityAsset: merging multiple instances, resolving conflicts, providing combined tooltip lines.

Item vs Item Set assets
- UYIInventoryAsset (single item)
  - Holds stacks (presentation, core stats, requirements, audio/VFX, size, abilities, economy, tags).
  - Serves as the authoring surface for an individual item.
- UYIItemSetAsset (set-level authoring)
  - SetName, Description, Icon; Members: array of UYIInventoryAsset soft refs.
  - SetBonuses: array of thresholds (e.g., 2-piece, 4-piece, 6-piece) → array of UYIAbilityAsset references applied when the character equips enough members.
  - Editor: a dedicated tab to manage set membership and bonuses; cross-links to member items.
  - Optional: Per-member overrides (e.g., set-flavored tooltip lines, set rarity tint).

Example mapping (Utraean Guard’s Plate)
- Presentation: Name = "Utraean Guard’s Plate", Rarity inferred (e.g., Epic/Legendary), Icon/Mesh assigned.
- Core stats: Defense = 434, Weight from item data.
- Requirements: Strength ≥ 55.
- Abilities stack:
  - UYIAbility_Modifier(+5 Strength).
  - UYIAbility_Proc(OnDamageTaken, Chance = 0.18, Effect = Nova Strike ability/effect asset, Cooldown as needed).
- Economy: Sell Value = 246736.
- Membership: UYIItemSetAsset "Mien of the Utraean Guard" includes this item; SetBonuses defined at 2/4/6 pieces as separate UYIAbilityAsset lists.

Editor UX implications
- Item editor tabs: Overview, Stacks, Abilities, Requirements, Audio/VFX, Size/Placement, Economy, Tags, Tooltip Preview.
- Add-from-template flow: Designers click "+" on a stack to pick a template (BP asset) or a predefined entry; entries are fully blueprintable and may expose preset lists (e.g., common procs/modifiers).
- Ability authoring: Ability assets have their own editor with preview text (using description templates and context), icon, and test hooks.

- Item asset (UYIInventoryAsset)
  - DisplayName, Description, Rarity, Icon.
  - DefaultSize (Rows, Columns), Stacking (bAllowStacking, MaxStackCount), bUniquePerType.
  - Types: ItemType, SlotType (tags/strings) for grid/slot matching.
  - Secondary overlay data: SecondaryIcon, Edge, Percent.
  - Capabilities: modular list for behaviors (Economy, Equip, Durability, Use, Ability, Crafting…)
- Capability examples
  - Economy: buy/sell/repair costs, currencies.
  - Equip: EquipSlots, Requirements (tags/level/attrs), Weight.
  - Durability: Current/Max, Break rules.
  - UseAction: Consumable effect, cooldown, charges.
  - Ability/GAS: granted ability class, tags, level.
  - SpellbookEntry: spell id/asset, school, rank, mana cost, learn/use rules.

## Equipment system requirements
- Equip slots
  - Define slot types (e.g., Head, Chest, WeaponMain, WeaponOff, Ring, Amulet) as tags.
  - Character exposes an equipment container with fixed slots; each slot accepts items with matching SlotType and valid Equip capability.
- Equip/unequip flow
  - Attempt equip for an item triggers validation:
    - Slot type match; character requirements satisfied; uniqueness/handedness (e.g., two-handed weapons occupying both hands) enforced.
  - On equip success:
    - Apply capability effects: tags, abilities, attribute changes, meshes/visuals.
  - On unequip:
    - Remove effects, drop back to inventory if space available.
- Two-handed and dual-wield
  - Equip capability can mark TwoHanded; system reserves both WeaponMain/Off. Dual-wield requires both hands free and item allowed.
- Weight/encumbrance (optional)
  - Sum Weight across equipped items; expose value for movement/stamina systems.

## Shops/Merchants requirements
- Merchant containers are store grids with IsStore=true.
- Buying
  - On pickup from store grid: validate funds (Economy capability), optionally taxes/discounts; then transfer into player bag using standard placement rules.
- Selling
  - Drag from player bag into store sell area; validate item sellability; add currency to player; optionally remove from world inventory.
- Repair (optional)
  - Items with Durability may be repaired for cost; UI stub triggers Economy.RepairCost.
- Price modifiers (optional)
  - Rarity, reputation, and vendor tags can adjust buy/sell multipliers.

## Spellbooks requirements
- Spells as items
  - SpellbookEntry capability references a Spell asset; item can be used to learn spell (consumed) or equip in spell slots.
- Learning flow
  - Use item in inventory; validate prerequisites (level, school, prior ranks); mark learned in player spellbook; consume item.
- Spellbook container (optional)
  - Grid or list of learned spells; supports hotbar assignment; not required for MVP.
- Using spells (integration)
  - If GAS is present, grant ability on learn; UI shows cooldown/cost from ability metadata.

## Integration and events mapping
- Inventory
  - OnAttemptPlace, OnDisplace, OnPlaced, OnPickupRequested, OnPickupContinue, OnAutoPlaced, OnInvalidPlacement.
- Equipment
  - OnAttemptEquip, OnEquipped, OnUnequipped, OnEquipDenied (reason), OnRemoveRequested.
- Shops
  - OnAttemptBuy, OnAttemptSell, OnPriceQuery, OnRepairRequested.
- Spellbooks
  - OnAttemptLearn, OnLearned, OnLearnDenied (reason), OnUseSpellItem.

## Editor requirements
- Item Editor
  - Core fields + capability cards; tooltip preview with rarity and capability summaries; secondary overlay preview.
- Inventory/Bag Editor
  - Grid with drag/rotate (if enabled), rollover tints, stack overlays, forward/reverse sort.
- Equipment Editor (Phase 2)
  - Define slot sets; preview character equip; validate Equip capability rules.
- Merchant Editor (Phase 2)
  - Store grids; price preview; buy/sell/repair stubs.

## Stack entry catalog (initial, extensible)

Designers compose an item by adding entry items to stacks. Each entry is a blueprintable template with fields and tooltip text. Examples:
- Name entry
  - Fields: DisplayName (FText), Flavor lines; optional rarity override.
- Set entry
  - Fields: ItemSet (UYIItemSetAsset ref); optional per-item set-note.
- Damage entry (for weapons)
  - Fields: DamageMin, DamageMax, DamageType tag; optional scaling/affixes.
- Requirements entry
  - Fields: Strength, Intelligence, Dexterity, Level, Tags; comparison mode (>=, ==).
- Sell Value entry
  - Fields: Value, CurrencyType.
- Proc entry (example: "10% chance to Petrify")
  - Fields: TriggerEvent (OnHit, OnDamageTaken, OnUse), Chance, Effect asset (e.g., Petrify), Cooldown, Conditions, Tooltip text.
- Defense/Armor entry (for armor)
  - Fields: Defense, ArmorType tag.
- Audio/VFX entry
  - Fields: DropSound, UseSound, EquipSound, FX references.
- Size/Placement entry
  - Fields: Rows, Columns, RotationAllowed, ItemType, SlotType, SecondaryOverlay (texture, edge, percent).
- Tags/Metadata entry
  - Fields: Gameplay Tags, category filters, search keywords.

Notes
- Entries are additive; the editor aggregates them to compute final item properties and tooltips.
- Conflicts are resolved by well-defined rules per entry type (e.g., last-wins or sum/merge based on entry class).

## DS1 ability import (CSV plan)
- Source: CSV listing DS1 abilities/effects and attributes (name, description, proc chance, trigger, effect data, modifiers, rarity hints).
- Importer tool:
  - Parses CSV rows into UYIAbilityAsset instances:
    - If row specifies a proc: create UYIAbility_Proc with trigger, chance, cooldown, effect mapping.
    - If row specifies a modifier: create UYIAbility_Modifier with attribute/value/scaling.
    - If row specifies a direct spell: create UYIAbility_GASWrapper or UYIAbility_RawEffect based on mapping rules.
  - Supports idempotent re-import (update existing assets by a stable key).
- Editor integration:
  - Palette shows imported abilities grouped by category/tags; designers drag-drop into an item’s Abilities stack.

## Aggregation and conflict rules (authoring → runtime)

General model
- Items aggregate values from multiple stack entries into a final runtime representation used by gameplay and tooltips.
- Each entry class declares its merge policy. Examples:
  - Name: last-wins (by authoring order) for DisplayName; flavor lines append.
  - Set: last-wins for Set ref; multiple set entries allowed only if explicitly enabled (otherwise validation error).
  - Damage: sums multiple components of different DamageType tags; same DamageType entries merge additively unless flagged exclusive.
  - Requirements: all constraints must be satisfied (AND). Same attribute constraints merge to the max threshold.
  - Sell Value: last-wins unless UseMax is ticked (then take max); currency must match bag context.
  - Proc: multiple procs co-exist; identical proc keys merge probabilities if MergeMode=Additive or collapse by highest if Exclusive.
  - Defense/Armor: additively summed unless Exclusive.
  - Audio/VFX: last-wins for each slot (Drop/Use/Equip); multiple VFX can play if flagged Additive.
  - Size/Placement: last-wins for Rows/Columns/RotationAllowed; ItemType/SlotType last-wins with validation.
  - Tags/Metadata: union of tags; duplicates removed.

Runtime overrides
- UYIStackInstance can apply runtime-only modifiers (e.g., durability loss, temporary buffs) layered on top of authored values.
- Abilities may modify final values during Equipping/Use by emitting additive/multiplicative deltas.

## Tooltip composition

Goals
- Human-friendly, DS1-like multi-line tooltip built from entries and abilities, with color coding and rarity tint.

Composition order (recommended)
1) Name line (rarity colored), Set membership inline or on second line.
2) Core stats: Damage/Defense and other primary numbers.
3) Requirements (colored red if unmet).
4) Abilities (modifiers and procs), grouped and sorted by importance.
5) Extra: weight, durability, special tags.
6) Sell Value at bottom.

Formatting
- Use a tooltip builder that requests lines from each entry and from each ability asset via GenerateTooltipLines(Context).
- Entries decide their own localization-ready text; abilities provide friendly descriptions.
- Apply iconography for procs/effects where useful.

## Example flows

Equip flow
- User drags item to an equip slot → validate SlotType and Requirements.
- On success: apply ability OnEquip hooks; update visuals and tooltip; reflect set bonuses via UYIItemSetAsset thresholds.

Auto-place and sort
- When bag is sorted: clear grid, place items row-first (forward or reverse); if failed, restore original layout.

Store buy/sell (Phase 2)
- Buy: pick up from store grid after price check → AutoTransfer into inventory using placement rules.
- Sell: drag from player bag to store; validate sellability; grant currency; item removed.

Spellbook learn (Phase 2)
- Use spellbook item: validate prerequisites → grant ability (GASWrapper or RawEffect) → consume item.

## CSV schemas (initial)

Abilities.csv
- Key, Type (Modifier|Proc|GAS|Raw), DisplayName, Description, IconRef, Tags, RarityHint,
- For Modifier: Attribute, Mode (Add|Mul), Value, ScaleByLevel
- For Proc: Trigger (OnHit|OnDamageTaken|OnUse), Chance, Cooldown, EffectKey, Conditions
- For GAS: AbilityClass, Level, Cooldown, Tags
- For Raw: ScriptKey/Path

Import behavior
- Create or update UYIAbilityAsset by Key; place in Content/YOLOInventory/Abilities/Imported.
- Log conflicts and validation warnings (missing fields, bad enums).

## Validation rules

Asset-level
- Item: DefaultSize >= 1x1; if bAllowStacking==false then MaxStackCount==1; UniquePerType optional independent.
- Size/Placement entry required for grid items; Presentation Name required.
- Damage vs Defense: at least one primary stat required depending on item category.
- Set entry: multiple sets disallowed unless explicitly enabled.

Runtime checks
- Placement: bounds, footprint, displacement rules.
- Equip: slot type match; requirements met; two-handed/dual-wield constraints.
- Abilities: UniquePerItem/Character respected; cooldown/conditions enforced.

## Roadmap and next actions

Immediate (coding)
- Implement stack entry base classes and a few core entries: Name, Size/Placement, Requirements, Damage, Defense, Sell Value, Proc.
- Add UYIAbilityAsset hierarchy (Modifier, Proc, GASWrapper, RawEffect) with tooltip builders.
- Extend UYIInventoryAsset to store stacks (template entries) and runtime cloning into UYIStackInstance.
- Update Bag to reference UYIStackInstance, support placement and stacking, and fire UI delegates.

Short-term (editor)
- Item editor tabs and add-from-template UX.
- Grid editor overlays and sort/auto-place control.
- Tooltip preview panel using builders from entries/abilities.

Mid-term
- Item Set asset + set bonuses application.
- CSV importer tooling and re-import support.
- Validation pass and automated tests.

## Milestones and checklist
- [ ] Mirror DS1 code/assets into repo (Plugins/YOLOInventory/DungeonSiege/...)
- [ ] Audit current YOLOInventory functionality vs design
- [ ] Finalize UYIInventoryAsset schema and base capabilities
- [ ] Implement UYIInventoryBag + FYIBagItem and rules (CanPlaceAt, FindFirstFit, stacking, rotate)
- [ ] Build Item Editor (capability cards, previews, validation)
- [ ] Build Bag Editor (grid, drag/rotate/split/combine, overlays, hints, AutoPack)
- [ ] Implement runtime API and WBP_BagGrid
- [ ] Validation rules and rich tooltips
- [ ] Tests (unit + editor)
- [ ] Save/Load
- [ ] Optional: occupancy grid & advanced auto-pack; replication

## Decisions/Notes (log)
- 2025-12-25: Drafted initial requirements and architecture plan based on DS1 behavior and current plugin goals.
- Next: Ingest DS1 code from DungeonSiege folders and revise rules to match exactly, then start schema stubs.

## DS1 source findings (ongoing)

Further DS1 details and flags (from GameGUI)
- Grid and placement flags (UIGridbox)
  - bIsStore: for store grids; triggers buyer-gold checks on pickup and alters pickup flow (requires request_pickup/continue flags).
  - bRequestPickup: if true, emits request_grid_pickup before continuing a pickup.
  - bContinuePickup: gate set by messenger handlers to allow/deny pickup/placement.
  - bAutoTransfer: when picking up, notifies check_inventory_space and then transfer_inventory for auto moves.
  - bAutoPlaceOnly: disallows manual placements unless override is true; auto placement mode.
  - bLocalPlace: controls whether local grid can place (used with auto-place and multi-bag flows).
  - m_bDeactivateOverlapped: dictates if the overlapped item remains active after PickupOverlapped.
  - m_bConsumesItems: when true, GridHitDetect with consumes path saves lastItem and sends grid_request_placement and item_placement to consume.
  - m_bAutoPlaceNotify: decides whether to notify grid_autoplaced after auto placement.
  - LastSort: SORT_FORWARD/SORT_REVERSE sort state preserved; AutoSort() uses reverse order.
- Visuals and interaction (UIGridbox)
  - Rollover: uses RolloverTimer, highlights prospective footprint cells in green/red; separate rollover highlight for an item under cursor.
  - Fullness metric: GetFullRatio() sums item Rows*Columns over grid capacity.
  - Scaling: SetScale rescales grid and items rects.
- Items (UIItem)
  - Size: rows, columns, box_width/height default to 32; no explicit rotate behavior present in this layer.
  - Secondary overlay: percent-visible with edge specification for variants/charges.
  - Stack visuals: prints stack number bottom-right if >1.
  - Type matching: grid.GetGridType() must equal item.GetItemType() for placement.
- Item slots (UIItemSlot)
  - Equip slots flagged via is_equip_slot; equip attempts emit itemslot_equip and depend on CanEquip set by game logic.
  - Removal from equip slots emits itemslot_request_remove and depends on CanRemove.
  - Non-equip slots copy texture and adjust UVs to show multi-cell items within slot bounds.

Open questions and assumptions (to validate or extend)
- Rotation: DS1 UI layer shows no rotation operations. Assumption: items are not rotated in DS1 inventory. Our plugin can support optional rotation per container as a modern extension.
- Stacking/combine semantic source: The UI layer maintains numItems and renders count, but combine/split logic appears to be handled via messenger/gameplay-side responses (e.g., when dropping on same-type). We will provide explicit Combine/Split APIs and fire events mirroring DS1 messages.
- Auto-pack: DS1 provides Sort()/AutoSort() placing items forward/reverse with first-fit row scanning; no Tetris packing. We will start with first-fit and optionally expose advanced packers.
- Store behavior: buyer gold checks and request_pickup/continue gates require higher-level logic. We will expose Blueprint events mirroring DS1 and allow user code to set ContinuePickup/CanEquip/CanRemove.
- Uniqueness rules: Not visible in this UI layer; expected to be enforced by gameplay messaging (e.g., deny additional unique stacks). We’ll include UniquePerType and Stacking settings in item asset schema and enforce in bag logic with events for veto/override.

Mapping to YOLOInventory plugin
- Data schema additions
  - Item: ItemType, SlotType, Rows/Columns default size, Stack visuals, SecondaryIcon with percent and edge.
  - Bag/Container: flags for IsStore, RequestPickup, AutoTransfer, AutoPlaceOnly, LocalPlace, DeactivateOverlapped, ConsumesItems, AutoPlaceNotify, LastSort, RolloverDelay.
- Runtime API and events
  - Implement DS1-style GridHitDetect with nearest-cell origin, bounds/footprint validation, and single-occupant displacement via PickupOverlapped hook.
  - Expose delegates/Blueprint events analogous to DS1 messages for buyer-gold checks, request_pickup, equip attempts, removal requests, and transfers.
  - Provide CombineStacks/SplitStack operations and fire appropriate events to let game logic accept/deny and adjust counts.
- Editor UX
  - Grid preview with DS1 rollover cell tinting and stack count overlay.
  - Item capability card including secondary overlay settings and item type/slot type selection.
  - Bag settings panel listing DS1-like flags with tooltips.


Additional extracted details (continuation):
- UIItem specifics
  - Constructor reads: rows, columns, item_type, slot_type, texture; font is optional.
  - Input flow: MSG_LBUTTONDOWN starts placement if dragging; MSG_LBUTTONUP commits placement if m_bPlaceValid and mouse moved; MSG_DEACTIVATEITEMS deactivates item.
  - CheckForPlacement:
    - Tries gridboxes first: if grid type matches item type, visible, and interface visible, calls GridHitDetect(left, top, this) if dragging and not JustPlaced.
    - Then tries item slots: if slot type matches and visible, calls ItemSlot::PlaceItem; if slot is equip but incompatible, sets AttemptEquipId and notifies "itemslot_incompatible".
    - Then tries info slots similarly.
  - ActivateItem toggles dragging/visibility/state, sends messages "activate_item" and MSG_ITEMACTIVATE; on deactivate removes from active items; maintains m_bPlaceValid.
  - SetMousePos moves item rect around the cursor with tolerance checks and updates m_bMouseMoved; only draws while active/visible.
  - Draw draws secondary overlay first, reloads textures if needed, draws base, then stack count bottom-right.
  - Secondary overlay: edge clipping with percent visible; supports all four edges; used to represent variable level/charge.
- UIItemSlot specifics
  - Equip slot: fhWindow.Get("is_equip_slot", m_bEquipSlot). Equip attempt path: sets AttemptEquipId, notifies "itemslot_equip"; external logic sets CanEquip, and placement aborts if false.
  - PlaceItem logic:
    - If JustPlaced, normalizes ownership and re-enables Grid ItemDetect on gridboxes, then returns false.
    - Non-equip slot: copies the item texture to slot.
    - Equip slot: triggers equip check via messenger; bNotify false path bypasses check.
    - If slot already has an item and bNotify: swaps it out (ActivateItem(true), reassign ID, RemoveItem, optional deactivate old).
    - Sets slot’s item, normalizes item scale, parents item to slot, adjusts UV to fit non-1x1 sized items.
    - After placement: re-enable grid item detect if no active items; equip slots toggle highlight based on active items; notifies "place_item" and MSG_ITEMPLACE.
  - RemoveItem: if equip slot, notifies "itemslot_request_remove" and requires CanRemove=true; activates item for pickup and clears slot state; notifies remove.
  - GetTextBox/PositionTextBox: tooltip behavior similar to grid rollover.
- UIGridbox specifics (tail end)
  - AutoSort uses reverse order; Sort will reset grid and re-place; if a re-place fails, it restores original layout via AssembleGrid.
  - SetScale rescales box sizes and item rects proportionally.
  - PositionTextBox and GetTextBox manage a shared tooltip textbox window.


Core UI inventory code (GameGUI):
- UIItem (ui_item.h/.cpp)
  - Draggable item icon used for inventory.
  - Grid occupation specified via Rows/Columns; supports SetRows/SetColumns.
  - Stack count overlay via SetNumStacked/GetNumStacked.
  - Item typing: ItemType and SlotType strings.
  - Activation state; color tint; secondary texture overlay (with percent visible and edge type) for item variants (e.g., potion level).
- UIItemSlot (ui_itemslot.h/.cpp)
  - Slot/rectangle that can be marked as equip slot (is_equip_slot flag read from UI data).
  - Equip flow: SetAttemptEquipId on slot and notify messenger ("itemslot_equip"); has GetCanEquip/SetCanEquip and GetCanRemove/SetCanRemove.
  - Visual polish: highlight/activate/special colors; rollover timer.
- UIGridbox (ui_gridbox.h/.cpp)
  - Grid control backing inventory.
  - Occupancy model: 2D array GridUnit[MAX_ROWS][MAX_COLUMNS] with bOccupied + itemID; per-item GridItem records (rect, alpha, numItems, secondary_percent, highlight colors).
  - Placement pipeline: GridHitDetect(x,y,pItem, bNotify,bSnap,bAutoPlace,bOverride)
    - Uses nearest cell by distance to mouse (PointDistance) to choose origin.
    - Validates bounds and checks occupancy across the W x H footprint derived from item Rows/Columns.
    - If cells occupied by a single different occupantID, calls PickupOverlapped to pick that item up (via messenger) unless auto-place or store rules disallow.
    - On success: fills occupancy cells with itemID, computes item rect from cell origin and item UI size, appends to m_grid_items, sets JustPlaced, and notifies messenger ("item_placement"; plus "item_pickup" if displacement occurred).
  - Pickup pipeline: ItemHitDetect() detects click on an item rect, sets pickupId, runs store/buyer checks and request_grid_pickup as needed, then activates the UIItem and clears its occupied cells.
  - Auto placement: AutoItemPlacement(item_name,bSnap,id,secPercent) scans grid row-first and tries GridHitDetect with bAutoPlace=true.
  - Rollover visuals: DrawRolloverBoxes highlights prospective cells green/red depending on bOccupied per cell of the item footprint at nearest origin.
  - Sorting: Sort(SORT_FORWARD/REVERSE) re-places items using AutoItemPlacement; AutoSort() uses reverse order.
  - Fullness: GetFullRatio() sums Rows*Columns of items / grid capacity.
- UIToolTip (ui_tooltip.h/.cpp)
  - Multi-line colored tooltip data (TextLineVec), printed with configured font/justification; shown after RolloverDelay in UIGridbox.

Implications for our plugin design:
- Placement algorithm
  - Choose nearest grid cell to the drop point and attempt placement there.
  - Validate bounds and ensure either all cells empty, or all cells belong to a single occupantID (displacement allowed) when not in autoplace/store modes.
  - If displacement: pick up the overlapping item first, honoring rules (e.g., buyer gold, request_pickup messaging) before placing the dragged item.
  - Maintain an occupancy map for fast overlap tests; store per-placed-item rect and visual metadata for UI rendering.
- Auto-place and sort
  - Row-first scanning for first-fit; optionally notify “grid_autoplaced.”
  - Provide forwards and reverse sorts that re-place in order.
- Stacking and overlays
  - Each placed item records numItems; draw stack number bottom-right if >1.
  - Support secondary overlay percent and edge type for item variants.
- Messaging/event hooks
  - Provide events analogous to DS1: check_inventory_space, transfer_inventory, item_pickup, item_placement, request_grid_pickup, ui_grid_item_rollover/rolloff, grid_autoplaced, convert_persist_items.
  - Our plugin should surface these as delegates/Blueprint events so gameplay code can attach modern logic.
- Tooltips
  - Build tooltip content as colored lines from item capabilities and rarity; show after configurable rollover delay; position around rollover rect.

