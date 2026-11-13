"""Sample the licensed Landscape in world coordinates for MuJoCo collision."""
from pathlib import Path
import re
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parents[1]
height=np.array(Image.open(root/'Content/Moon/Texture/heightmap_ue.png'),dtype=np.float64)
def elevation(x,y):
    u=(np.asarray(x)*100+100456.56)/50;v=(np.asarray(y)*100+100782.16)/50
    j=np.floor(u).astype(int);i=np.floor(v).astype(int);fx=u-j;fy=v-i
    a,b,c,d=height[i,j],height[i,j+1],height[i+1,j],height[i+1,j+1]
    h=np.where(fx<fy,a+(d-c)*fx+(c-a)*fy,a+(b-a)*fx+(d-b)*fy)
    return (86.05+(h-32768)/128*50)/100
axis=np.linspace(-150,150,1501);x,y=np.meshgrid(axis,axis);z=elevation(x,y)
low,span=float(z.min()),float(np.ptp(z))
output=root/'Content/RuntimeTerrain';output.mkdir(exist_ok=True)
((z-low)/span).astype('<f4').tofile(output/'terrain_hfield.bin')
p=root/'Content/model/go2/scene.xml';s=p.read_text()
s=re.sub(r'size="150 150 [^"]+"',f'size="150 150 {span:.9f} 0.001"',s)
s=re.sub(r'pos="0 0 -[\d.]+"',f'pos="0 0 {low:.9f}"',s)
s=re.sub(r'qpos="0 0 [-\d.]+ ',f'qpos="0 0 {float(elevation(0,0))+.35:.9f} ',s)
p.write_text(s)
print('Collision patch: 300 x 300 m; 0.2 m samples; MuJoCo gravity remains 9.81 m/s2')
