# Inventory Grid Shader Library

This plugin now registers a shader source mapping:

- Virtual path: `/Plugin/YOLOInventory`
- Physical path: `Plugins/YOLOInventory/Shaders`

Shader files:

- `/Plugin/YOLOInventory/InventoryGrid/YIInventoryGridCommon.ush`
- `/Plugin/YOLOInventory/InventoryGrid/YIInventoryGridThemes.ush`

## Designer workflow (fast setup)

1. Create a new **Material** for UI (Domain: `User Interface`, Blend: `Translucent`).
2. Add a **Custom** node.
3. In the Custom node:
   - Add include path:
     - `/Plugin/YOLOInventory/InventoryGrid/YIInventoryGridThemes.ush`
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

4. Feed output to material color/opacity.
5. Make **Material Instances**:
   - `MI_Grid_Fantasy` (`ThemeBlend=0`)
   - `MI_Grid_SciFi` (`ThemeBlend=1`)
   - tune scalar params per game.
6. Assign MI into your `UYIInventoryGridStyleAsset` brush slots (cell, hover, selection, ghost overlays).

## Notes

- These helpers are intentionally lightweight (simple SDF + pulse + dotted marquee).
- You can make variable shape borders with `YI_RoundedRectBorder` + `YI_DottedMarquee`.
- For mobile/low-end:
  - lower dot counts
  - reduce sine frequencies
  - disable marquee in non-hover states.

