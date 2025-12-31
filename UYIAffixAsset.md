# UYIAffixAsset — Affix / Magic Modifier Asset

Status: Proposed

## Overview ✅
`UYIAffixAsset` represents a single affix (prefix/suffix/implicit) used for Dungeon Siege-style item magic generation. An Affix is an authorable asset that references or contains attribute modifiers (stat deltas) and carries generation metadata (tier, weight, roll ranges, allowed targets, conflict group, tooltip formatting, etc.).

The goal is to keep attribute application logic (existing `UYIAttributeModAsset`) reusable while providing the extra metadata and authoring surface required for randomized affix generation and tooling.

---

## Requirements (Why we need it) 🎯
- Data-driven affix definitions (prefix/suffix) for loot generation and uniques.
- Authoring metadata to control generation: weight, tiers, allowed item tags, conflict groups, and roll rules.
- Reuse existing attribute modifier assets where possible to avoid duplicating stat logic.
- Deterministic (seedable) generation for testability and reproducibility.
- Small, serializable per-instance representation (`FYIAffixInstance`) to persist rolled values.

Pools and generation
- `UYIAffixPoolAsset` is a lightweight asset that lists candidate affixes with weights, level ranges and optional minimum quality filters.
- Item definitions may expose `PrefixPool` and `SuffixPool` to drive randomized affix generation.
- Affixes have a `Quality` (Common/Magic/Rare/Unique) which can be used to limit sampling by item quality or power bands; pool entries can also require a minimum quality.
- Create pools from Content Browser: Add New -> YOLO Inventory -> Affix Pool (registered by the editor module).
- The runtime API `UYIInventoryBlueprintLibrary::GenerateAffixesForInstance` can be used to deterministically roll affixes for an instance using a seed and desired prefix/suffix counts.
- Editor tooling for previewing rolls and validating pools.

---

## Minimal Asset Schema (fields) ✍️
Below is a compact schema suggested for the asset.

```cpp
class UYIAffixAsset : public UObject
{
    // Stable identity
    FName InternalId;           // stable internal id
    int64 UniqueCode = 0;       // optional global numeric code (editor unique)

    // Presentation
    FText DisplayName;
    FText Description;          // short rich text

    // Classification
    enum EAffixKind { Prefix, Suffix, Implicit } Kind;
    int32 Tier = 1;             // relative power group
    float Weight = 1.0f;        // sampling weight inside pools

    // Effect references
    TArray<TSoftObjectPtr<UYIAttributeModAsset>> AttributeMods; // applied when affix attached

    // Generation rules
    float MinValue;             // simple min/max roll
    float MaxValue;
    FRuntimeFloatCurve ValueByLevel; // optional scaling curve

    // Constraints
    FGameplayTagContainer AllowedItemTags; // which item defs can receive this
    FName ConflictGroup;             // prevents multiple affixes from same group

    // Display & tooling
    FText TooltipFormat;         // e.g., "+{0}% Damage"
    Editor-only: SampleRoll(seed) -> example values
};
```

---

## Per-instance representation (minimal) 🔧
`FYIAffixInstance` attaches to `FYIItemInstance` and is small and serializable.

```cpp
struct FYIAffixInstance
{
    TSoftObjectPtr<UYIAffixAsset> Source;
    int32 TierRolled;
    float RolledValue;       // or typed values array
    uint32 Seed;             // used for deterministic roll reproduction
    FName ConflictGroupCache;
    FText DisplayNameCache;  // optional localized name variant
};
```

---

## Pools & Generation
- Introduce `UYIAffixPoolAsset` (list of affix entries with per-entry weight, min/max level filters).
- Generation service samples pools, respects `AllowedItemTags` and `ConflictGroup`, enforces max prefix/suffix counts, and fills `FYIItemInstance.Affixes`.
- Support deterministic generation via optional seeds and deterministic RNG.

---

## Editor UX & Tools 🔧
- Affix Editor: preview sampled values, view referenced attribute mods, mark conflict groups and allowed tags.
- Pool Editor: visualize weights, sample preview, constraints (max prefixes/suffixes).
- Migration helper: convert commonly-used `UYIAttributeModAsset` into a draft `UYIAffixAsset` (one-click wrapping) to ease transition.

---

## Integration notes
- Use existing capability system (`UYICapability`) to apply affix-driven attribute changes if necessary, or apply attribute mods directly in `FYIItemInstance` construction.
- Keep AttributeMod assets first-class; `UYIAffixAsset` should reference them rather than reimplementing attribute logic where feasible.

---

## Save/Migration & Versioning 🔁
- `FYIAffixInstance` must be versioned in save formats.
- Provide migration code that finds prefixed attributes without affix assets and wraps them into new `UYIAffixAsset` objects where possible.

---

## Testing
- Unit tests for sampling determinism (seed-based), pool weight distribution, conflict-group enforcement, and level-based scaling.

---

## TODOs / Next steps
- Author `UYIAffixAsset` C++ header + default UAsset factory
- Implement `UYIAffixPoolAsset` and generator service
- Add basic Slate editor widgets and sample-roll visualization
- Add unit tests for generator determinism and pool behavior

---

If you'd like, I can draft a compact C++ header sketch for `UYIAffixAsset` and `FYIAffixInstance`, and a short PR checklist to add the asset and pool support.