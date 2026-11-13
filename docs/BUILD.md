# 从源码构建

需要 Linux x86_64、Unreal Engine 5.7、Python 3，以及配套月面场景源工程（不随代码仓库提供）。设置 `UE_ROOT` 为包含 `Engine/` 的引擎目录。

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements-build.txt
python scripts/import_assets.py /path/to/MoonProject
export UE_ROOT=/path/to/UnrealEngine_5.7
./scripts/build.sh
```

MuJoCo 和 ONNX Runtime Linux 库、Go2 模型和 ONNX 权重已随源码依赖提供。`scripts/import_assets.py` 只从你提供的工程复制需要的地图素材，并从 Poly Haven 下载 CC0 碎石纹理。

`build.sh` 会生成共享碰撞高度场，编译编辑器模块，烘焙外观，再生成 Linux 包。输出目录为 `dist/Linux`。地图和生成的材质、碰撞数据均由 `.gitignore` 排除。

准备完成后，可以用编辑器运行或调试：

```bash
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PWD/LunarG1.uproject" /Game/Moon/Level/Moon -game -windowed -ResX=1280 -ResY=720 -nocef -ansimalloc -norhithread
```

内部 UE 模块名称保留为 `LunarG1`；产品和 GitHub 仓库名称为 LunarGo2。运行程序不依赖用户目录中的固定路径。
