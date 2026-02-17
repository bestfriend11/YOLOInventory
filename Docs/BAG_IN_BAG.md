# Bag-in-Bag (Nested Containers)

## What is implemented

- Container items can own a nested runtime bag.
- Nested bags are identified by `ContainedBagId` on item instances.
- Active bag content replication stays lazy:
  - all bag descriptors replicate (owner-only),
  - only the active bag item payload replicates (`NetBagItems`).
- Client opens nested bags via server RPC (`OpenContainedBagAtIndex`) and server switches active bag.

## Item setup

On `UYIItemDefinition`:

- Enable `bIsContainerItem`.
- Optionally assign `ContainerTemplateBag` for predefined layout/content.
- Otherwise set `ContainerDefaultGridSize`.

Container items are forced to non-stackable runtime behavior.

## Runtime navigation

- `OpenContainedBagAtIndex(ItemIndex)` opens nested bag from active bag item.
- `OpenParentBag()` navigates one level up.
- Inventory screen action menu adds `Open` for container items.
- Cancel input first tries `OpenParentBag()` before closing workflow.

## Replication behavior

- Owner-only descriptors (`NetBagDescriptors`) include:
  - `ParentBagId`
  - `ParentItemInstanceId`
  - `bIsNestedContainer`
  - `ItemCount`
- Active bag payload only (`NetBagItems`) includes nested link per item (`ContainedBagId`).

This avoids eager replication of all nested contents.
