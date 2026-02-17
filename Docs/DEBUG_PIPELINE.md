# YOLOInventory Debug Pipeline

This plugin now routes runtime diagnostics through one central pipeline (`UYIDebugLibrary`).
Designers can control noise from **Project Settings** and **console commands** without code edits.

## Project Settings

Open:

- `Project Settings -> Plugins -> YOLO Inventory -> Debug|Pipeline`
- `Project Settings -> Plugins -> YOLO Inventory -> Debug|Channels`

Key controls:

- `bEnableDebugPipeline`: master on/off
- `bDebugOutputToScreen`: on-screen text output
- `bDebugOutputToLog`: Output Log routing
- `bAllowForcedDebugMessages`: allows "critical/forced" messages
- `bDebugDeduplicateMessages` + `DebugDuplicateIntervalSeconds`: spam protection
- channel toggles (`Persistence`, `ActionBar`, `Inventory`, etc.)

## Console Commands

- `YOLOInventory.Debug.Status`
- `YOLOInventory.Debug.Profile off|minimal|normal|verbose`
- `YOLOInventory.Debug.Pipeline 0|1|toggle`
- `YOLOInventory.Debug.Screen 0|1|toggle`
- `YOLOInventory.Debug.Log 0|1|toggle`
- `YOLOInventory.Debug.Force 0|1|toggle`
- `YOLOInventory.Debug.Channel <all|general|persistence|inventory|equipment|actionbar|trade|shop|grid|phase2> 0|1|toggle`
- `YOLOInventory.Debug.History`
- `YOLOInventory.Debug.History clear`

Recommended quick presets:

- stop noisy preflight immediately: `YOLOInventory.Debug.Channel persistence 0`
- keep only critical logs: `YOLOInventory.Debug.Profile minimal`
- full runtime troubleshooting: `YOLOInventory.Debug.Profile verbose`

## Debug UI / Visual Debugger Hooks

Use `UYIDebugRouterSubsystem` in Blueprint:

1. Get `GameInstanceSubsystem` of type `YIDebugRouterSubsystem`.
2. Bind to `OnDebugMessage`.
3. Render custom debug window / visual overlays from received `FYIDebugMessageRecord`.

Buffered history is available via:

- `UYIDebugLibrary::GetDebugMessageHistory`
- `UYIDebugRouterSubsystem::GetBufferedMessages`

This keeps runtime output extensible for custom in-game debug HUDs.
