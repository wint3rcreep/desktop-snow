from pathlib import Path
import plistlib,struct
from PIL import Image
root=Path(__file__).resolve().parents[1]
p=plistlib.loads((root/'Info.plist').read_bytes())
assert p['CFBundleShortVersionString']=='3.3' and p['CFBundleIconFile']=='DesktopSnow.icns'
for n in (16,32,128,256,512):
 for scale in (1,2):
  f=root/'Assets/DesktopSnow.iconset'/f'icon_{n}x{n}{"@2x" if scale==2 else ""}.png'
  with Image.open(f) as im:assert im.size==(n*scale,n*scale);im.verify()
data=(root/'Assets/DesktopSnow.icns').read_bytes();magic,length=struct.unpack('>4sI',data[:8]);assert magic==b'icns' and length==len(data)
i=8;chunks=[]
while i<len(data):
 tag,n=struct.unpack('>4sI',data[i:i+8]);assert n>=8 and i+n<=len(data);chunks.append(tag.decode());i+=n
assert i==len(data) and 'ic10' in chunks
with Image.open(root/'Assets/DesktopSnow.icns') as im:assert any(w*scale==1024 and h*scale==1024 for w,h,scale in im.info['sizes']);im.load()
print('PASS plist3.3,icon key,10 PNG sizes16–1024,ICNS length/chunks/1024 decode:',','.join(chunks))
# C structs retained end-to-end, no numeric array reinterpretation.
s=(root/'Sources/main.swift').read_text();assert 'var sprites:[SnowSprite]' in s and 'var piles:[SnowPile]' in s
assert 'drawLooseSnow(s, context)' in s and 'path.addQuadCurve' in s and 'guard world.intersects(rect)' in s
assert 'cp "$ICON" DesktopSnow.app/Contents/Resources/DesktopSnow.icns' in (root/'build.command').read_text()
print('PASS static Swift typed-buffer/plain-clump/bounds-culling and app-icon packaging contracts (NOT AppKit compilation)')

assert 'for curve in c.contours' in s and 'snow_contours(nil,0)' in s
assert 'bank?.addCurve' in s and 'curve.start != 0' in s and 'curve.end != 0' in s
assert 'WindowCornerInset' in s and 'snow_corner_inset(assumedInset)' in s
print('PASS native renderer consumes C++ exported segmented cubic geometry and configurable corner profile (static only)')
