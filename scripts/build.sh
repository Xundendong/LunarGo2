#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."
: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine 5.7 directory}"
python scripts/prepare_collision.py
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" LunarG1Editor Linux Development "$PWD/LunarG1.uproject" -WaitMutex
mkdir -p Binaries/Linux
cp -a ThirdParty/mujoco/lib/*.so* ThirdParty/onnxruntime/lib/*.so* Binaries/Linux/
env -u LD_LIBRARY_PATH "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PWD/LunarG1.uproject" /Game/Moon/Level/Moon \
  "-ExecutePythonScript=$PWD/scripts/prepare_scene_ue.py" -nocef -ansimalloc -norhithread -unattended -RenderOffscreen
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$PWD/LunarG1.uproject" \
  -noP4 -unattended -platform=Linux -clientconfig=Development -build -cook -stage -pak -archive \
  -archivedirectory="$PWD/dist" -map=/Game/Moon/Level/Moon -utf8output -nodebuginfo

cp run.sh README.md LICENSE THIRD_PARTY_NOTICES.md dist/Linux/
cp -a licenses dist/Linux/
