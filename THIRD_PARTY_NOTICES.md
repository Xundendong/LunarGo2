# Third-party materials

| Component | Source | Terms |
|---|---|---|
| Unreal Engine 5.7 runtime | Epic Games | Unreal Engine EULA; not covered by this repository's MIT license |
| Moon Landscape Open World | [Epiqua / Fab](https://www.fab.com/listings/dea3878b-b56c-4c3b-880a-34dbe3ef778e) | Commercial asset; editable assets excluded from Git. Incorporated only in the cooked application; no standalone redistribution permission is granted |
| Gravel Stones textures | [Poly Haven](https://polyhaven.com/a/gravel_stones), also used by OmniLRS | CC0; https://polyhaven.com/license |
| Go2 model, deployment convention and MoE policy | [go2_rl_gym](https://github.com/wty-yy/go2_rl_gym), local revision `30e74dc507bec7a642a8c98be26081f2c6f0822d` | MIT / Unitree BSD-3-Clause; full notices in `licenses/GO2_RL_GYM.txt` |
| MuJoCo 3.11.0 | Google DeepMind | Apache-2.0; `licenses/MUJOCO.txt` |
| ONNX Runtime 1.20.1 | Microsoft | MIT and bundled dependency notices; `licenses/ONNXRUNTIME*.txt` |
| OmniLRS reference | [OmniLRS](https://github.com/OmniLRS/OmniLRS) | BSD-3-Clause; `licenses/OMNILRS.txt` |

The ONNX checkpoint is the converted `go2_moe_cts_high_slope_thre_164k_0.6715.pt` policy, with explicit observation history. It is a terrestrial-gravity locomotion policy, not a policy trained for the Moon.

The standalone integration uses public MuJoCo/Unreal/ONNX APIs. It does not bundle the previous third-party MuJoCoUE plugin or any Unreal Editor binaries.

The binary distribution is a compiled application, not an open asset library. Commercial environment content and its derived collision data remain under their original terms. You need your own licensed environment to rebuild that map from source.

Unreal® Engine, Copyright 1998–2026, Epic Games, Inc. All rights reserved. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.

The packaged application and its incorporated third-party components are provided “as is”. To the maximum extent permitted by law, all representations, warranties, conditions and liabilities relating to the Unreal Engine materials are expressly disclaimed. This distribution grants no right to extract or redistribute the engine or commercial assets as standalone products.
