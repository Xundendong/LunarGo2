"""Import a legally acquired Moon project and download CC0 gravel textures."""
from pathlib import Path
import argparse
import shutil
import urllib.request

parser=argparse.ArgumentParser()
parser.add_argument('moon_project',type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
for name in ['Moon','__ExternalActors__/Moon','__ExternalObjects__/Moon']:
    source=args.moon_project/'Content'/name
    if source.exists():shutil.copytree(source,root/'Content'/name,dirs_exist_ok=True)
assert (root/'Content/Moon/Level/Moon.umap').exists(), 'Expected the Epiqua Moon Landscape Open World project'
textures=root/'private_assets/gravel';textures.mkdir(parents=True,exist_ok=True)
for name,ext in [('diff','png'),('nor_dx','exr'),('rough','exr'),('ao','png')]:
    filename=f'gravel_stones_{name}_4k.{ext}'
    target=textures/filename
    if not target.exists():
        urllib.request.urlretrieve(f'https://dl.polyhaven.org/file/ph-assets/Textures/{ext}/4k/{filename}',target)
    print(target.name)
