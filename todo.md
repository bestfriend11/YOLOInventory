# YOLOInventory – Execution Plan (Data‑Driven, UE Plugin)

This todo translates the system_goal.md into actionable milestones. The goal is a fully data‑driven, extensible inventory plugin that can implement Dungeon Siege–like systems without hard‑coding DS specifics.

## Phase 0 — Stabilization and UX polish (in progress)
- [ ] Finalize cross‑grid drag visuals (per‑grid ghost vs global overlay toggle)
- [ ] Ensure “displace and continue dragging (unattached)” is consistent for same‑bag and cross‑bag drops (option flag)
- [ ] Trading screen sample UMG (Left/Right grids + tooltips) and blueprint example
- [ ] Tooltips: ensure RefreshBoundTooltip covers selection/hover/drag states

## Phase 1 — Schema extensions (core data model)
- [ ] Item Definition: add modular Stat Components array (UItemStatComponent base; see below)
- [ ] Item Instance: DynamicStats (GameplayTag→float), CurrentDurability, CurrentCharges, StackCount
- [ ] Affix Asset: AllowedItemTags (GameplayTagQuery), LevelRange, SpawnWeight, ConflictTags
- [ ] Keep current Affix rolls (Min/Max, ValueByLevel, PowerLevel) and apply to DynamicStats

### Stat Components (first wave)
- [ ] WeaponStat: DamageTypes (min/max per type), AttackSpeed, Range, ProjectileRef
- [ ] ArmorStat: ArmorRating, Resistances map
- [ ] ShieldStat: BlockChance, BlockAmount
- [ ] CustomScalarStat: arbitrary Tag→float pairs for project‑defined stats

## Phase 2 — Requirement system (pluggable rules)
- [ ] Define IYIRequirement interface (CanEquip/CanUse with reason codes)
- [ ] Implement basic Requirement assets:
  - [ ] RequiredLevel
  - [ ] AttributeGate (tagged attributes with thresholds)
  - [ ] Class/Membership Gate (GameplayTagQuery on pawn)
  - [ ] SlotCompatibility (Two‑handed vs OffHand / slot tags)
- [ ] Add array of Requirement assets on Item Definition
- [ ] Blueprint API: EvaluateEquip/Use with user‑friendly failure reasons

## Phase 3 — Effects and GAS integration
- [ ] Item Definition: OnUseAbility/OnUseEffect references (soft), optional OnEquip/OnUnequip
- [ ] Equip pipeline: grant/remove ability sets or apply effects; map DynamicStats→Attributes
- [ ] Use pipeline: activate ability/effect (potions, books, traps)
- [ ] Sample content: potion ability and a simple “teach spell” book

## Phase 4 — Loot & generators (data assets)
- [ ] LootTable Asset: entries (weights, tag/level filters, rarity profiles, quantities), nested tables
- [ ] Generator Asset: links LootTable + Level provider; optional container fill rules
- [ ] Deterministic seeding via FRandomStream; multiplayer‑friendly APIs
- [ ] Editor preview tool: simulate N rolls at level/seed

## Phase 5 — UI/Tooling
- [ ] Tooltip renderer that queries Stat Components and DynamicStats (formatter per component)
- [ ] InventoryGridWidget options: behavior flags (swap vs displace, exact placement enforcement)
- [ ] Validation: asset audit (missing refs, bad tag queries, invalid stat/req combos)
- [ ] Content Browser category and AssetTypeActions for new assets

## Phase 6 — Optional importer (DS sample → data assets)
- [ ] Minimal .gas/.skrit parser to map a subset of DS templates to our generic assets (no hard‑coding)
- [ ] Conversion guide: field mapping tables

## Acceptance (aligned with system_goal.md)
- Items are built from components, affixes, tags, curves—no game‑specific code required
- Requirements, effects, and generators are data assets and interfaces
- Tooltips and UI are driven by component descriptors and rules
- Dungeon Siege sample can be represented without plugin code changes

## Notes / References
- See system_goal.md for design principles and DS reference files under:
  - Plugins/YOLOInventory/DungeonSiege/world
