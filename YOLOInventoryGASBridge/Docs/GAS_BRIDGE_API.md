# YOLOInventoryGASBridge: First API Surface and Boundaries

Status: Phase-1 API definition (runtime stubs + contract)

## 1) Plugin Boundary

`YOLOInventoryGASBridge` is the only suite plugin that should hard-depend on GAS.

- **Depends on:** `GameplayAbilities`, `GameplayTags`, `YOLOInventoryCore`, `YOLOInventorySchema`
- **Must not depend on:** Grid/UI/Shop/Trade/World editor modules
- **Purpose:** item-fragment-driven GAS grant/use/description bridge

This keeps core/schema/container modules non-opinionated and GAS-agnostic.

## 2) Public API Surface (v1)

### 2.1 Runtime request/result contract

Header: `Source/YOLOInventoryGASBridge/Public/YIGASBridgeApiTypes.h`

- `FYIGASBridgeOpResult`
- `EYIGASBridgeOpKind`
- `EYIGASBridgeOpError`
- `FYIGASBridgeRequestContext`
- `FYIGASBridgeGrantRequest`
- `FYIGASBridgeActivateRequest`
- `FYIGASBridgeDescriptionRequest`
- `FYIGASBridgeDescriptionToken`

Key rules:

- Uses canonical item identity: `FYIInventoryItemRef` (bag + instance id).
- Supports context tags + evaluation level + difficulty scale.
- Returns structured result envelope for server-safe workflows.

### 2.2 Service entrypoint

Header: `Source/YOLOInventoryGASBridge/Public/YIGASBridgeSubsystem.h`

- `UYIGASBridgeSubsystem::RequestApplyItemGrants`
- `UYIGASBridgeSubsystem::RequestRemoveItemGrants`
- `UYIGASBridgeSubsystem::RequestActivateItem`
- `UYIGASBridgeSubsystem::BuildDescriptionTokens`

Current implementation intentionally returns `NotImplemented` after basic request validation.
This is by design for staged rollout.

## 3) GAS Fragments (v1 schema in bridge plugin)

Header: `Source/YOLOInventoryGASBridge/Public/YIGASBridgeFragments.h`

- `FYIItemGASGrantDefinitionFragment` (static definition)
- `FYIItemGASUseEffectDefinitionFragment` (static definition)
- `FYIItemGASScalingDefinitionFragment` (static definition, uses `FScalableFloat`)
- `FYIItemGASRuntimeStateFragment` (runtime instance state)

Notes:

- `FScalableFloat` is used only for GAS-facing scalable magnitudes.
- Runtime mutable state remains concrete values (timestamps/counters/tags).

## 4) Ownership and Call Graph

- Equipment/use systems build request structs and call bridge subsystem.
- Bridge resolves fragments from schema snapshot and runtime instance.
- Bridge applies GAS grants/effects on server-authoritative ASC.
- Description engine can query `BuildDescriptionTokens` and merge into tooltip sections.

No UI module should mutate GAS directly.

## 5) Security and MMO Constraints

- Server authoritative activation/grant checks only.
- Client can request; server validates item identity + context + tags/cooldown.
- Avoid heavy per-frame GAS reflection in UI; use cached tokens from bridge.
- Runtime replication remains inventory-driven; do not replicate full GAS snapshots per item.

## 6) Next Implementation Steps

1. Implement `ApplyItemGrants_Internal` and `RemoveItemGrants_Internal`:
   - resolve `FYIItemGASGrantDefinitionFragment`
   - load abilities/effects async-safe (prewarm where possible)
   - grant/remove via ASC handles
2. Implement `ActivateItem_Internal`:
   - resolve `FYIItemGASUseEffectDefinitionFragment`
   - enforce cooldown using `FYIItemGASRuntimeStateFragment`
3. Implement `BuildDescriptionTokens_Internal`:
   - evaluate `FScalableFloat` using request context
   - output localized tokens for tooltip/panel renderers
4. Wire equipment/use plugins through this subsystem only (no direct GAS writes).

