# YOLO Inventory Suite: Architecture and Extension Path

This document is the source of truth for keeping the suite decoupled, non-opinionated, and extensible.

## 1) Layering (dependency direction)

Allowed dependency direction:

`Core -> Schema -> Containers/Grid/World/Equipment/Trade/UI -> EditorCore -> Editor*`

Optional/legacy layer:

`LegacyBridge` (compatibility types/assets only; do not depend on it from core systems).

## 2) Plugin responsibilities

- `YOLOInventoryCore`
  - Base settings, debug routing, core types.
  - No DS1/template-specific runtime rules.
- `YOLOInventorySchema`
  - Item/affix schema, fragments, registry, mapping utilities.
  - Authoring model only; no UI opinion.
- `YOLOInventoryContainers`
  - Bag/component/container runtime logic.
- `YOLOInventoryGrid`
  - Grid style/runtime helpers + grid shader mapping.
  - Canonical shader virtual root: `/Plugin/YOLOInventoryGrid`.
- `YOLOInventoryWorld`
  - Pickups/drop actors and world-loot integration.
- `YOLOInventoryEquipment`
  - Equipment/action bar systems.
- `YOLOInventoryTrade`
  - Shop/trade session systems.
- `YOLOInventoryUI`
  - Runtime widgets and drag/drop presentation.
- `YOLOInventoryEditorCore`
  - Shared editor extension points/factories.
- `YOLOInventoryEditorSchema/Grid/Loot`
  - Domain editor tooling split by responsibility.
- `YOLOInventoryLegacyBridge`
  - Migration shims and legacy asset types (`YIGridContainer`, `YIItemBlueprintLibrary`, etc.).
- `YOLOInventoryTemplateDS1`
  - Opinionated DS1 showcase/template content.

## 3) Non-opinionated rule set

Must stay out of `Core` and `Schema`:

- Game-specific slot taxonomies.
- DS1-specific generator or loot semantics.
- Presentation-theme assumptions (dark fantasy/scifi, etc.).
- Template/sample-only assets.

Place these in:

- `TemplateDS1` for showcase behavior.
- New project-specific extension plugin(s) for studio behavior.

## 4) Runtime extension path

Create a new runtime plugin (example: `YOLOInventoryMyGame`), then depend only on required suite modules.

Typical `Build.cs` dependencies:

- Always: `YOLOInventoryCore`, `YOLOInventorySchema`
- Add as needed: `YOLOInventoryContainers`, `YOLOInventoryEquipment`, `YOLOInventoryTrade`, `YOLOInventoryWorld`, `YOLOInventoryUI`
- Optional migration-only: `YOLOInventoryLegacyBridge`

Guideline:

- Extend by composition (new components/subsystems/assets), not by editing suite internals.

## 5) Editor extension path

Use `YOLOInventoryEditorCore` extension points first.

Current extension points already available:

- Bag dashboard bridge factory
- Generator dashboard bridge factory

Recommended pattern for new editor plugin:

1. Depend on `YOLOInventoryEditorCore` + your required runtime/editor domain modules.
2. Register your factory/bridge in module `StartupModule`.
3. Unregister in `ShutdownModule`.
4. Keep mode-specific tools in dedicated editor plugin (schema/grid/loot-style split).

## 6) Shader extension path

Grid shader library is owned by `YOLOInventoryGrid`:

- Physical: `Plugins/YOLO/YOLOInventoryGrid/Shaders/InventoryGrid`
- Virtual (canonical): `/Plugin/YOLOInventoryGrid/InventoryGrid/...`
- Legacy virtual root is still mapped for compatibility.

For new themes:

1. Add `.ush` in `YOLOInventoryGrid/Shaders/...`
2. Include from material custom node via canonical virtual path.
3. Drive state parameters via `UYIInventoryGridStyleAsset`.

## 7) Decoupling checklist for new changes

Before merging:

1. No new `Build.cs` dependency from suite runtime modules to legacy `YOLOInventory`.
2. No plugin descriptor dependency from suite plugins to legacy `YOLOInventory`.
3. New gameplay opinions placed in `TemplateDS1` or project extension plugin, not `Core/Schema`.
4. Full editor build passes.
