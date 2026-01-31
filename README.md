# YOLOInventory

A modular, network-ready inventory & trading plugin for Unreal Engine. It provides grid/list bags, runtime item instances, trading flows, autosave, and plug-and-play UI widgets.

## Highlights
- Grid/list bags with rotation, filters, stack merging, thumbnails, tooltips.
- Runtime-safe bag cloning (assets stay pristine) with lightweight replicated mirrors.
- Multiplayer-ready item movement, pickup, and trading between players or NPCs.
- Autosave/restore on `PlayerState` via `UYIPlayerInventoryStateComponent` (async, debounced).
- UMG/Slate widgets: inventory grids, drag overlay, shared tooltip, trading screen.
- Blueprint-friendly components plus C++ extension points and validation hooks.

## Folder layout
- `Plugins/YOLOInventory/Source/YOLOInventory`          � runtime systems (bags, items, networking, trade).
- `Plugins/YOLOInventory/Source/YOLOInventoryEditor`    � editor dashboards & helpers.
- Key headers to start with:
  - `YIInventoryComponent`            � attach to pawns; holds the equipped bag and replicates a preview mirror.
  - `YIPlayerInventoryStateComponent` � add to PlayerState; owns party/shared bags, resources, autosave.
  - `YITradeInteractionComponent`     � add to PlayerController; client->server trade requests, UI hook.
  - `YITradeSessionActor`             � lightweight replicated session that mirrors both sides� inventories.
  - `InventoryGridWidget` / `TradingScreenWidget` � UMG logic for grids and trading UI.

## Quick setup (multiplayer safe)
1) **Enable plugin** in your UE project and restart the editor.
2) **PlayerState**: add `UYIPlayerInventoryStateComponent`. Leave `bEnableAutoSave` on for autosave (uses `SaveSlotName`).
3) **Pawn**: add `UYIInventoryComponent`. In GameMode (server), assign a runtime bag per pawn (clone a template or create with `CreateBag`) and call `OpenBag` after possession.
4) **PlayerController**: add `UYITradeInteractionComponent`. Optionally set `bAutoShowWidget` + `AutoTradeWidgetClass` (TradingScreenWidget subclass).
5) (Optional) **UI**: place `InventoryGridWidget` / `TradingScreenWidget` in your UMG. Bind tooltip if desired.

## Giving each player their own bag (example)
```cpp
// Server-side, after possession
auto* InvState = PC->GetPlayerState()->FindComponentByClass<UYIPlayerInventoryStateComponent>();
auto* InvComp  = PC->GetPawn()->FindComponentByClass<UYIInventoryComponent>();
UYIInventoryBag* RuntimeBag = DuplicateObject(TemplateBag, InvState); // or CreateBag
int32 Slot = InvState->AddPartyMember({ PC->GetPawn()->GetClass(), RuntimeBag, FText::FromString("Hero") });
InvState->AssignInventoryToPawn(PC->GetPawn(), Slot);
InvComp->OpenBag(RuntimeBag);
```

## Trading flow
- Client calls `RequestTrade(Target, bTargetIsNPC)` on `YITradeInteractionComponent`.
- Server validates and spawns `AYITradeSessionActor`, mirroring both inventories (including grid sizes).
- Clients receive `OnTradeSessionReady` (or auto UI shows if enabled). Left = local inventory, Right = other side.
- Inventory and offer mirrors replicate live; moving items updates both views immediately.

## Persistence
- `UYIPlayerInventoryStateComponent` autosaves on bag `OnChanged` (debounced) using async SaveGame.
- Loads on server `BeginPlay`, restores when a pawn is available. Works in PIE listen server + clients.

## Tips / gotchas
- Always assign bags on the **server**. Clients cannot author bags.
- Assets are cloned at runtime to keep templates clean; don�t reuse one bag asset for multiple players.
- For UI correctness, ensure `OpenBag` is called after assigning a runtime bag so grids bind and replicate.
- Net mirrors now include grid size (`NetBagGridSize`) to keep layouts consistent client-side.

## Console helpers
- `yi.additem <code> <count>` (server) can be used to spawn items into the current bag (if enabled in your build).
