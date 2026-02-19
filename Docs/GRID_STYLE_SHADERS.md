# Inventory Grid Shader Library

`YOLOInventoryGrid` now registers shader source mappings:

- Canonical virtual path: `/Plugin/YOLOInventoryGrid`
- Backward-compatible virtual path: `/Plugin/YOLOInventory`
- Physical path: `Plugins/YOLO/YOLOInventoryGrid/Shaders`

Shader files:

- `/Plugin/YOLOInventoryGrid/InventoryGrid/YIInventoryGridCommon.ush`
- `/Plugin/YOLOInventoryGrid/InventoryGrid/YIInventoryGridThemes.ush`

## Designer workflow (fast setup)

1. Create a new **Material** for UI (Domain: `User Interface`, Blend: `Translucent`).
2. Add a **Custom** node.
3. In the Custom node:
   - Add include path:
     - `/Plugin/YOLOInventoryGrid/InventoryGrid/YIInventoryGridThemes.ush`
     - (or legacy) `/Plugin/YOLOInventory/InventoryGrid/YIInventoryGridThemes.ush`
   - Add inputs:
     - `UV` (`float2`)
     - `Time` (`float`)
     - `HoverAmount` (`float`)
     - `SelectedAmount` (`float`)
     - `InvalidAmount` (`float`)
     - `MarqueeAmount` (`float`)
     - `ThemeBlend` (`float`) // 0=fantasy, 1=scifi
   - Output type: `CMOT Float4`
   - Custom code:

```hlsl
float4 fantasy = YI_GridTheme_DarkFantasy(UV, Time, HoverAmount, SelectedAmount, InvalidAmount, MarqueeAmount);
float4 scifi = YI_GridTheme_SciFi(UV, Time, HoverAmount, SelectedAmount, InvalidAmount, MarqueeAmount);
return YI_GridTheme_Blend(fantasy, scifi, ThemeBlend);
```

Optional advanced variant (choose marquee path):

```hlsl
// MarqueeMode: 0 TL->BR, 1 TR->BL, 2 Border CW, 3 Border CCW
float marqueeMode = 2.0;
float4 fantasy = YI_GridTheme_DarkFantasyEx(UV, Time, HoverAmount, SelectedAmount, InvalidAmount, MarqueeAmount, marqueeMode);
float4 scifi = YI_GridTheme_SciFiEx(UV, Time, HoverAmount, SelectedAmount, InvalidAmount, MarqueeAmount, marqueeMode);
return YI_GridTheme_Blend(fantasy, scifi, ThemeBlend);
```

4. Feed output to material color/opacity.
5. Make **Material Instances**:
   - `MI_Grid_Fantasy` (`ThemeBlend=0`)
   - `MI_Grid_SciFi` (`ThemeBlend=1`)
   - tune scalar params per game.
6. Assign MI into your `UYIInventoryGridStyleAsset` brush slots (cell, hover, selection, ghost overlays).

## Zero-headache automation (recommended)

You do **not** need to manually wire hover/selected/invalid parameters per widget.

In `UYIInventoryGridStyleAsset`:

- enable `bAutoDriveThemeMaterialParameters`
- set `ThemeBlend` (0 fantasy, 1 sci-fi, or blend)
- keep/default parameter names:
  - `ThemeBlend`
  - `Time`
  - `HoverAmount`
  - `SelectedAmount`
  - `InvalidAmount`
  - `MarqueeAmount`
  - `SlotStateId` (optional)

Then just assign the material instance to style slots.  
At runtime, grid rendering pushes state values automatically for each slot state.

Typical mapping done by grid:

- hover cell/item -> `HoverAmount=1`
- selected cell -> `SelectedAmount=1`
- invalid ghost placement -> `InvalidAmount=1`
- marquee-capable states -> `MarqueeAmount=1`
- theme switching -> from style `ThemeBlend`

## Notes

- These helpers are intentionally lightweight (simple SDF + pulse + dotted marquee).
- Marquee variants available in `YIInventoryGridCommon.ush`:
  - `YI_DottedMarqueeDiagTLToBR(...)`
  - `YI_DottedMarqueeDiagTRToBL(...)`
  - `YI_DottedMarqueeBorderClockwise(...)`
  - `YI_DottedMarqueeBorderCounterClockwise(...)`
- You can make variable shape borders with `YI_RoundedRectBorder(...)` + any marquee variant.
- For mobile/low-end:
  - lower dot counts
  - reduce sine frequencies
  - disable marquee in non-hover states.
