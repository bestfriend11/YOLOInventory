# YOLOInventory System Goals and Requirements

## Vision

YOLOInventory is a general-purpose, extensible inventory framework for Unreal Engine. It must enable building a wide variety of inventory-driven games entirely through editor-authored data, without hard‑coding game‑specific rules. Dungeon Siege data included in this repository is an example content set to validate capabilities — not something the plugin hardcodes.

## Design Principles

- Data‑Driven: Behavior is governed by Data Assets, Gameplay Tags, and Curves — not fixed code paths.
- Modular Composition: Items are composed from reusable "Stat Components" and optional Affixes.
- Extensible & Pluggable: Requirements, Generators, and Effects are swappable assets/interfaces.
- GAS First-Class: On‑use/on‑equip behaviors integrate with Gameplay Ability System (GAS).
- UI‑Agnostic but Friendly: Core widgets expose clean APIs; projects can skin/extend freely.
- Multiplayer‑Ready: Deterministic generation (seeds/streams), and replication-friendly instances.

## Core Data Model

### Item Definition (DataAsset)
Represents the static template for an item family.
- Identity: Id/Name, Icon, Mesh, Description.
- Inventory: Footprint (grid size), RotationsAllowed, Weight, Value, Stackable (MaxStack), DurabilityMax, ChargesMax.
- Categorization: Gameplay Tags (e.g., Item.Weapon.Sword, Item.Armor.Plate, Item.Usable.Potion).
- Equip Model: Equip Slot tags (Equip.Head, Equip.MainHand, Equip.OffHand, Equip.TwoHand, etc.).
- Requirements: Array of Requirement Assets (see below).
- Stat Components: Array of UItemStatComponent assets (modular stats below).
- Generation: RarityProfile, AffixProfile, LevelRange, AllowedAffixTags, MinRandomModifiers/MaxRandomModifiers.
- Effects: OnUseAbility/OnUseEffect (GAS), optional OnEquip/OnUnequip hooks.

### Item Stat Components (DataAsset, polymorphic)
Composable building blocks for item stats. Example components:
- WeaponStat: DamageTypes (min/max per type), AttackSpeed, Range, ProjectileRef.
- ArmorStat: ArmorRating, Resistances (per damage type).
- ShieldStat: BlockChance, BlockAmount.
- Socketing: SocketCount, AllowedSocketTypes.
- CustomScalarStat: TMap<GameplayTag, float> for arbitrary project-defined stats.
Projects can add new components without modifying the plugin.

### Affix Asset
Parametric modifiers applied at generation.
- Roll Range: MinValue/MaxValue, ValueByLevel curve, PowerLevel multiplier.
- Filters: AllowedItemTags (GameplayTagQuery), LevelRange, SpawnWeight, ConflictTags.
- Outputs: Stat deltas (e.g., Tag->float), optional OnHit/OnEquip GAS refs.

### Item Instance (runtime)
- Reference to Definition.
- Rolled Affixes and values (seeded, deterministic).
- DynamicStats (Tag->float), CurrentDurability, CurrentCharges, StackCount.
- Optional component state (if components need runtime state).

## Requirements & Rules (Data‑Driven)
Requirement assets implement interfaces such as:
- CanEquip(Actor, ItemInstance, out Reason)
- CanUse(Actor, ItemInstance, out Reason)
Examples: RequiredLevel, AttributeGate, Class/Membership Gate (by tags), SlotCompatibility (Two‑handed vs Offhand), ConsumableAvailability.

## Effects & Abilities (GAS)
- OnUseAbility/Effect on Item Definition (and/or Affixes).
- Optional OnEquip/OnUnequip abilities/effects.
- Mapping of DynamicStats to Attributes and Ability Sets is project‑configurable.

## Loot Generation
- LootTable Asset: entries with weights, tag/level filters, rarity profiles, quantities; supports nesting and conditions via GameplayTagQuery.
- Generator Asset: references LootTable and a Level provider; container rules (e.g., fill chest/bag).
- Deterministic seeds for multiplayer.

## UI/UX
- Grid Inventory Widgets: support footprints, rotation, hover/ghost placement, validation, and drag‑drop across grids (including trading screen).
- Drag Ghost Policy: per‑grid or optional single global overlay; configurable behavior for displace vs. swap, and exact placement vs. first‑fit.
- Tooltips: component- and tag‑driven. Each Stat Component provides descriptive lines; affixes contribute rolled values; requirements provide unmet reasons.

## Extensibility Hooks
- Stat Components: add new stat types without plugin changes.
- Requirements: add new gating rules via assets.
- Generators: extend loot generation logic/data.
- Importers (optional): convert external datasets (e.g., Dungeon Siege .gas/.skrit) into our generic assets — no hard‑coded logic.

## Validation & Editor Utilities
- Asset validation (missing slots, invalid tag queries, unresolved references).
- Preview Generator: simulate loot at a given level/seed in-editor.
- Bulk import via CSV/JSON data tables.

## Multiplayer & Determinism
- Seeded FRandomStream usage for item generation.
- Minimal replicated state in Item Instance; recompute display from deterministic inputs when possible.

## Compatibility with Dungeon Siege‑like Content
The included .gas/.skrit files represent a complex item ecosystem (weapons, armor, skills, potions, books, containers, generators). YOLOInventory should represent these via:
- Categories & Stat Components (weapons, armor, shields, projectiles, sockets).
- Requirements (level/class/membership) via taggable assets.
- Effects (potions, spells, traps) via GAS abilities/effects.
- Generators & LootTables mirroring DS pools/weights.
No DS‑specific logic should live in the plugin; all behavior must arise from assets/tags/curves.

### Dungeon Siege reference files (relative paths)
These files are provided as a sample content set to calibrate the feature scope of the plugin. They must not be hard‑coded; instead, they should be mappable to data assets and tag/curve rules.

- Core catalogs and content database:
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/pcontent.skrit
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/pcontent.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/defaults.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/membership_seed.gas
- Components (behaviors, common properties):
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/components/components.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/components/chests/base_chest.skrit
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/components/generators/generator_basic.skrit
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/components/generators/generator_random.skrit
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/components/generators/generator_object_pcontent.skrit
- Templates (item archetypes):
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/_core/templates.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/wpn_sword.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/wpn_bow.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/wpn_staff.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/wpn_dagger.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/amr_body_plate.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/amr_helm.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/ptn_potion.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/trs_ring.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/trs_amulet.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/interactive/ctn_container.gas
  - Plugins/YOLOInventory/DungeonSiege/world/contentdb/templates/regular/generators/generator.gas

These reference files imply features like item categories, stat schemas, requirements, generators, containers, and UI descriptors — all of which must be achievable via the data‑driven systems described in this document.

## Roadmap (Suggested)
1) Schema: Stat Component framework; Requirement assets; extend Item Definition/Instance.
2) Generation: Rarity/Affix profiles; tag‑filtered affix selection; deterministic seeding.
3) Effects: OnUse/OnEquip via GAS; sample potion/ability integration.
4) Loot: LootTable + Generator assets; simple container rules.
5) UI: Tooltip renderer consuming components and requirements; optional global ghost overlay.
6) Tools: Validation, preview generator, optional importer prototype.

## Non‑Goals
- Hard‑coding rules from any specific game.
- Tying to a single combat system; instead we expose clean integration points (GAS, tags, attributes).

---

This document describes what the system must enable. Implementation details should follow these principles so that similar, but different, game designs can be authored without code changes.
