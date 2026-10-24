# PacManGame

in terms of using neovim 
1. use clangd for intelisense
    a. for creating compilecommand.json open power shell and paste this command 
    (
        & "C:\_dev\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -mode=GenerateClangDatabase -project="C:\Users\ahani\Documents\Unreal Projects\PacManGame\PacManGame.uproject" PacManGameEditor Win64 Development -OutputDir="C:\Users\ahani\Documents\Unreal Projects\PacManGame"
    )
    use correct path and regerate it when you change build.cs or add module or changing build setting in unreal 
2. tab / ctrl-n : is for auto complete