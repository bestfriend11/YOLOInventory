# Inventory Schema Roadmap (Multiplayer-safe, MMO-ready)

This roadmap migrates the plugin from single-active-bag mirroring to a fully consistent, exploit-resistant, ID-based inventory schema without breaking current gameplay.

## Current state (kept working)

- Server authority for mutations is already in place.
- Active bag is owner-only mirrored via `NetBagItems` + `NetBagGridSize`.
- UI and gameplay still work with current active-bag flows.

## Phase 1 (implemented now)

- Add stable bag identity on runtime objects:
  - `UYIInventoryBag::BagId`
  - `UYIInventoryBag::BagRoleTag`
- Add lookup APIs on inventory component:
  - `GetBagById`
  - `GetBagByRoleTag`
  - `GetBagByDisplayName`
- Add owner-only replicated bag descriptors:
  - `NetBagDescriptors` (`BagId`, display/role/grid, active flag)
- Keep legacy active-bag mirror untouched for backward compatibility.

## Phase 2

- Introduce ID-first UI binding:
  - Grids bind by `BagId`/`BagRoleTag`, not raw `Bags` index.
  - Spellbook/equipment widgets use descriptors and lookup APIs.
- Add explicit active contexts:
  - `ActiveBagId`
  - `ActiveSpellbookBagId`

### Phase 2 status (current)

- `UYIInventoryComponent` now tracks and replicates owner-only:
  - `ActiveBagId`
  - `ActiveSpellbookBagId`
- `UInventoryGridWidget` now supports runtime binding by:
  - explicit `BagId`
  - role tag (`BagRoleTag`)
  - active context (`ActiveBagId` / `ActiveSpellbookBagId`)
- `AYIPhase2TestMapActor` now sets spellbook active context with `SetActiveSpellbookBagById`.

## Phase 3

- Multi-bag delta replication:
  - owner-only fast-array records per bag/item delta (by `BagId`, `InstanceId`).
  - maintain bandwidth discipline by only sending changed entries.
- Remove hidden dependencies on `EquippedBag` for cross-bag operations.

## Phase 4

- Transaction layer + exploit hardening:
  - operation IDs (idempotency/replay protection)
  - server-side atomic validation for move/equip/sell/buy/swap
  - structured reject reasons + audit logging

## Phase 5

- Persistence abstraction for seamless disk -> database switch:
  - define `IInventoryPersistenceProvider`
  - keep current SaveGame provider as default implementation
  - add future DB provider without changing gameplay code paths

### Phase 5 status (current)

- Added `IYIInventoryPersistenceProvider` abstraction and base UObject provider.
- Added default backend `UYISaveGameInventoryPersistenceProvider`.
- `UYIPlayerInventoryStateComponent` now routes save/load/exists through provider APIs.
- SaveGame stays the default backend for now; DB provider can be added later without gameplay-layer rewrites.

## Persistence transition rule

- Gameplay code must only call persistence through provider interface.
- SaveGame remains current backend during development.
- Database backend can be enabled later by provider swap (no schema rewrite in gameplay layer).
