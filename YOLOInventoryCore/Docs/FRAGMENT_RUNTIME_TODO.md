# Fragment Runtime TODO

Status: Active design backlog

Goal: make fragments "alive" by driving runtime behavior (shop, trade, GAS, UI descriptions) without coupling core modules to feature opinions.

## 1) Architecture Rules (must keep)

- `YOLOInventoryCore` and `YOLOInventorySchema` stay generic and non-opinionated.
- Feature behavior lives in feature plugins (`Shop`, `Trade`, `Equipment`, `Loot`, `World`, `UI`).
- Fragment data ownership and runtime behavior ownership are separate:
  - data shape in `Schema` (or feature plugin if feature-specific)
  - behavior in feature plugin services/resolvers
- No direct UI mutation of gameplay state; UI asks command APIs only.

## 2) Fragment Categories and Plugin Ownership

### A. Economy / Commerce (YOLOInventoryShop)

- `FYIItemPriceDefinitionFragment` (static)
  - one or more currencies/resources
  - buy value, sell value, optional min/max clamps
  - optional scaling by quality/level/count
- `FYIItemShopPolicyFragment` (static)
  - buyable/sellable flags
  - visible in shop flag
  - optional allow-list / deny-list tags
- `FYIItemPriceRuntimeFragment` (dynamic)
  - temporary discounts, dynamic multipliers, event modifiers

Required runtime consumers:
- `YIShopPriceResolver` (server authoritative price calculation)
- `YIShopVisibilityResolver` (filter what appears in stock UI)
- shop buy/sell pipeline must use resolver output (not hardcoded listing-only logic)

### B. Trade Rules (YOLOInventoryTrade)

- `FYIItemTradePolicyFragment` (static)
  - tradable flag
  - trade context restrictions
- `FYIItemBindStateFragment` (dynamic)
  - account-bound / character-bound / trade-lock timers

Required runtime consumers:
- trade offer-add validation
- trade commit validation

### C. Equipment and Item Runtime State (YOLOInventoryEquipment)

- `FYIItemEquipRequirementsFragment` (static)
  - stat/tag requirements
- `FYIItemDurabilityRuntimeFragment` (dynamic)
- `FYIItemChargesRuntimeFragment` (dynamic)
- `FYIItemCooldownRuntimeFragment` (dynamic)

Required runtime consumers:
- equip request validation
- item-use validation and mutation

### D. Loot / Generation (YOLOInventoryLoot)

- `FYIItemLootEligibilityFragment` (static)
- `FYIItemRollPolicyFragment` (static)
- optional runtime roll-state fragment where needed

Required runtime consumers:
- loot generator pipeline
- container/chest roll logic

### E. World/Pickup Policy (YOLOInventoryWorld)

- `FYIItemPickupPolicyFragment` (static)
  - ownership, auto-pickup, decay/despawn behavior

Required runtime consumers:
- pickup actor interaction validation

### F. GAS Bridge (new plugin: YOLOInventoryGASBridge)

Why separate plugin: keep hard GAS dependency out of core suite modules.

- `FYIItemGASGrantFragment` (static)
  - abilities/effects/tags granted while equipped/active
- `FYIItemGASUseEffectFragment` (static)
  - effect(s) applied on use/consume
- `FYIItemGASScalingFragment` (static)
  - item-level / quality scaling parameters
- `FYIItemGASRuntimeStateFragment` (dynamic)
  - persistent per-item runtime state needed across equip/logout (timestamps, counters)

Required runtime consumers:
- equipment equip/unequip hooks
- consumable/use command path
- persistence integration for dynamic GAS-related runtime state

## 3) Description Engine (fast, fragment + GAS driven)

Target behavior:
- tooltip/description generated from fragment data + GAS metadata
- same text logic used at runtime and in editor preview
- no per-frame heavy reflection/string formatting in hot UI paths

Implementation TODO:
- add `YIDescriptionResolver` service (new plugin: `YOLOInventoryUI` or `YOLOInventoryDescription`)
- input: item definition snapshot + item instance runtime fragments + context
- output: cached structured description lines (not one giant string)
- cache key:
  - definition identity (`UniqueCode`)
  - instance identity (`InstanceId`) for runtime-dependent lines
  - revision/version counters for invalidation
- formatter support:
  - numeric formatting rules
  - localized text templates
  - rich text style tokens
  - GAS attribute/effect display adapters

Performance constraints:
- avoid runtime `LoadSynchronous()` in hover path
- resolve/calc once per revision, reuse for UI frames
- keep server authoritative values when text impacts transactions

## 4) Editor Preview and Designer Guidance

Requirements:
- when adding/removing fragments in editor, preview final item description immediately
- preview shows required/optional fields and unresolved fields
- preview includes shop price and GAS-derived lines when applicable

Implementation TODO:
- add "Description Preview" panel in schema dashboard/details
- add per-fragment docs metadata surfaced as rich tooltips + field hints
- add validation messages:
  - missing required fields
  - conflicting fragments
  - unsupported fragment combinations for selected feature modules

## 5) Runtime Integration Pattern (make fragments alive safely)

Introduce feature resolvers/adapters instead of hardcoded branching in item/bag classes:

- `IYIItemFeatureResolver` (conceptual interface, per feature plugin)
  - can evaluate item visibility/eligibility
  - can compute derived values (price, requirements, description tokens)
  - can validate mutation requests

Registration model:
- feature plugin registers resolver in module startup
- consumers query resolver registry by feature key/context

This keeps extension path open without editing core classes each time.

## 6) Security and MMO Constraints

- all economic and state-changing checks on server authority
- client tooltip can be predicted, but buy/sell/equip uses authoritative resolver output
- replication:
  - replicate only needed dynamic fragments
  - owner-only where possible
  - delta-oriented mirrors for large bags
- avoid O(n) scans in hot paths; use cache/index maps that rebuild on revision changes

## 7) Delivery Phases

### Phase 1 (first executable slice)
- add shop price + shop policy fragments
- implement `YIShopPriceResolver` + `YIShopVisibilityResolver`
- wire buy/sell to resolver output
- add tooltip price line from resolver

Status notes:
- Implemented with fixed-point integer pricing (basis points), not float-only economy values.
- Added optional level/quality/count scaling in price rules.
- Added GUID-targeted buy/sell request fields for safer stock/source identity resolution.
- Listing precedence: explicit listing prices override fragment prices when `bListingsOverrideFragmentPrices=true`.

### Phase 2
- add GAS bridge plugin + initial GAS fragments
- wire equip/use hooks to GAS bridge

Status notes:
- Added equipment-owned runtime fragments:
  - `FYIItemDurabilityRuntimeFragment`
  - `FYIItemChargesRuntimeFragment`
  - `FYIItemCooldownRuntimeFragment`
  - `FYIItemEquipRequirementsFragment`
- Added trade-owned fragments:
  - `FYIItemTradePolicyFragment`
  - `FYIItemBindStateRuntimeFragment`
- Added first runtime `YIItemDescriptionResolver` pass in UI plugin with cache + fragment line augmentation (shop/equipment/trade context).

### Phase 3
- ship description resolver with caching + editor preview panel

Status notes:
- Added rich grouped tooltip sections (`Requirements`, `Effects`, `Attributes`, `Condition`, `Economy`) built from resolved tooltip payload.
- Added subtitle metadata (`Type • Rarity`) derived from schema classification.
- Upgraded default tooltip Slate widget to a game-like layout (icon, subtitle, section headers, durability bar).
- Added Blueprint helper (`BuildRichTooltipForBagItem`) for list/panel UIs outside grid hover flow.

### Phase 4
- trade/equipment/loot/world policy fragments migrated to resolver pattern

### Phase 5
- optimization and contract tests for fragment-driven request/result paths

## 8) Immediate Open Items

- [x] Define exact `PriceDefinition` fragment fields and defaults
- [x] Decide listing precedence: fragment baseline vs listing override
- [ ] Add shop price/visibility contract tests
- [x] Add description preview panel in editor schema dashboard
- [ ] Define first GAS bridge API surface and plugin boundaries

Editor status note:
- Item dashboard preview now includes selection-level description + resolved fragment field preview (schema-driven, plugin-agnostic).
