# Inventory API Standard

Status: Active (phased rollout)

This document defines the suite-wide API contract for inventory runtime operations. The goal is:

- safe and auditable server-authoritative behavior
- consistent method signatures across plugins/views
- non-opinionated UI/topology support
- predictable performance for MMO-scale inventories

## 1) Canonical Identity (required)

All authoritative item operations use:

- `BagId` (`FGuid`)
- `ItemInstanceId` (`FGuid`)
- wrapped as `FYIInventoryItemRef`

Index-based operations are UI-local convenience only and must not be treated as canonical across RPCs/trade/multi-bag workflows.

## 2) Read API vs Command API

- Read/query methods are side-effect free (`Get*`, `Find*`, `Build*Ref`)
- Mutation methods are commands (`Move`, `Transfer`, `Split`, `Combine`, `Lock`, `Context`)
- Views (Grid/List/Tile/etc.) should call command APIs or command libraries, not mutate bags directly

## 3) Standard Request/Result Envelope

Core request/result types live in:

- `YOLOInventoryCore/Public/YIInventoryApiTypes.h`

Implemented request structs:

- `FYIInventoryMoveItemRequest`
- `FYIInventoryRotateItemRequest`
- `FYIInventoryRemoveItemRequest`
- `FYIInventoryTransferItemRequest`
- `FYIInventorySplitStackRequest`
- `FYIInventoryCombineItemRequest`

Standard result:

- `FYIInventoryOpResult`
- `EYIInventoryOpError`

Notes:

- `bRequestAccepted` means accepted locally / queued to server
- `bSucceeded` is authoritative success when executed on authority
- clients typically rely on replication for final state reconciliation

## 4) Authority Rules

- Client calls `Request*` or bag-targeted APIs
- Server validates and applies authoritative state
- Replication mirrors drive owning-client UI

Do not let UI assume local mutation success is final.

## 5) Revision / Concurrency Rule

Each runtime bag has a monotonic `RuntimeRevision`.

Use cases:

- optimistic UI validation
- stale request detection
- debugging desync cases

Current rollout status:

- `UYIInventoryBag.RuntimeRevision` implemented
- mirrored revisions for active/context bags implemented
- request wrappers can perform authority-side expected revision checks
- full RPC-level expected-revision propagation is pending (future phase)

## 6) Topology Independence (contract)

The API contract must not force a specific view type (grid/list/hive/slot).

Current state:

- command signatures are identity/bag-centric (good)
- grid/topology specifics still exist in some runtime container implementations (transitional)

Future requirement:

- container topology/placement policy abstraction in container runtime

## 7) Performance Constraints

For MMO-scale scenarios (including very large grids such as `100x100`):

- never replicate cells
- replicate item entries + placement state only
- owner-only UI mirrors for active/visible bags
- prefer delta replication for mirrors (future phase)
- avoid `LoadSynchronous()` in hot-path command execution

## 8) Security / Validation Checklist (server)

Every authoritative mutation path should validate:

- bag exists and is accessible
- item identity resolves (`BagId + ItemInstanceId`)
- lock state
- acceptance rules
- containment cycle prevention (bag-in-bag)
- stack and placement rules
- optional expected revision checks

## 9) Developer Usage Guidance

Preferred Blueprint path:

1. Build `FYIInventoryItemRef` via `UYIInventoryCommandLibrary`
2. Use by-ref command helpers or `UYIInventoryComponent::Request*`
3. Listen to inventory events / replication-driven UI updates

Avoid new usage of deprecated active-bag index-based mutation methods except for compatibility.

## 10) Phased Migration Plan (ongoing)

Phase A (done / in progress)

- canonical item refs standardized (`FYIInventoryItemRef`)
- bag-targeted mutation APIs added
- command library by-ref helpers added
- request/result core types added
- bag runtime revision added and mirrored for UI

Phase B (next)

- propagate expected revisions through RPC request paths
- standardize request/result wrappers across more systems (trade/shop/equipment actions)
- add service-level regression tests for mutation + mirror + container runtime

Phase C

- delta replication strategy for bag mirrors
- topology policy abstraction (grid/list/hive placement backends)
- async-loading enforcement in runtime command hot paths

