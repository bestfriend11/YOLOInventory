@echo off
REM Build MyProject4Editor and ShaderCompileWorker using UnrealBuildTool
REM Ensure these paths are valid on your machine before running.

dotnet "D:\Repos\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" -Target="MyProject4Editor Win64 Development -Project=\"D:\aa\MyProject4-571\MyProject4.uproject\"" -Target="ShaderCompileWorker Win64 Development -Project=\"D:\aa\MyProject4-571\MyProject4.uproject\" -Quiet" -WaitMutex -FromMsBuild -architecture=x64
