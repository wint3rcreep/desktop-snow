"""Portable source/packing contracts, NOT a Metal compiler or GPU screenshot test."""
from pathlib import Path
import ctypes, re, math, struct, zipfile
root = Path(__file__).resolve().parents[1]
s = (root/'Sources/MetalParticles.swift').read_text()
m = (root/'Sources/Particles.metal').read_text()
a = (root/'Sources/main.swift').read_text()
b = (root/'build.command').read_text()
def check(value, label):
    assert value, label
    print('PASS '+label)
# Inspect both production declarations, not a test-only translated particle type.
swift_fields = re.findall(r'var (\w+): SIMD4<Float>', s.split('struct MetalParticle {')[1].split('}')[0])
metal_fields = re.findall(r'float4 (\w+);', m.split('struct Particle {')[1].split('}')[0])
check(swift_fields == metal_fields == ['positionRadiusAlpha','angleKind'], 'production Swift/MSL record field order and float4 ABI (32 bytes, offsets 0/16)')
check('MemoryLayout<MetalParticle>.stride == 32' in s and 'offset(of: \\.angleKind) == 16' in s, 'native runtime ABI assertions guard actual Swift layout')
record = struct.pack('<8f', -1920.5, -44, 2.25, .6, 1.2, 1, 0, 0)
check(len(record) == 32 and struct.unpack_from('<f',record,16)[0] > 1.19, 'serialized test sentinel offset/stride')
check('[[buffer(0)]]' in m and '[[buffer(1)]]' in m and 'setVertexBuffer(buffer, offset: 0, index: 0)' in s and 'index: 1)' in s, 'shader argument table matches encoder particle/uniform slots')
check('vertexCount: 6, instanceCount: snapshot.count' in s and 'corners[6]' in m and '[[instance_id]]' in m, 'one batched six-vertex instanced quad draw')
check('float3(0.96,0.985,1.0)*alpha,alpha' in m and '.oneMinusSourceAlpha' in s and 'sourceRGBBlendFactor = .one' in s and 'sourceAlphaBlendFactor = .one' in s, 'premultiplied shader output and source-over RGB/alpha blend contract')
check('fwidth(radius)' in m and 'sin(angle)' in m and 'time*0.35' in s, 'analytic pixel derivative coverage and time/physical-angle rotation')
check('loadAction = .clear' in s and 'MTLClearColorMake(0,0,0,0)' in s and 'override var isOpaque: Bool { false }' in s and 'layer.isOpaque = false' in s, 'transparent drawable and fresh zero-clear every frame including empty frames')
check('waitUntilCompleted' not in s and 'available.wait(timeout: .now())' in s and s.index('worker.async') < s.index('layer.nextDrawable()') and 'maximumDrawableCount = 2' in s, 'nonblocking main-thread signal; drawable acquisition off-main; bounded one-frame buffer ownership')
check('capacity = 9300' in s and '16_777_216' in s and '8192' in s and 'makeTexture' not in s, 'bounded particles and per-monitor drawable dimensions; no offscreen textures')
check('override func hitTest' in s and '-> NSView? { nil }' in s and 'acceptsFirstResponder: Bool { false }' in s and 'p.ignoresMouseEvents=true' in a and 'event.keyCode == 53' in a, 'Metal child cannot steal interaction/first responder; existing panel default and Escape retained')
check('if metal != nil && (s.kind == 0 || s.kind == 5) { continue }' in a and 's.kind == 0 || s.kind == 5' in s and 'for curve in c.contours' in a and 'drawAccessories(s)' in a and 'drawTractor(t, context)' in a, 'exclusive particle ownership; AppKit cubic banks/snowman/tractor preserved')
check('MetalParticlesEnabled' in a and 'UserDefaults.standard.set(metalRequested' in a and 'metalFailed(error.localizedDescription)' in a and 'v.metal = nil' in a and 'completed.status == .error' in s, 'persistent opt-in and setup/GPU-error fallback to compatibility renderer')
check(all('-framework '+f in b for f in ['Metal','MetalKit','QuartzCore']) and 'cp Sources/Particles.metal' in b and 'device.makeLibrary(source:' in s, 'native framework link and bundled runtime shader compilation path')
# v3.0 intentionally changes CPU mechanics; retain the v2.9 GPU and monitor partition.
baseline = root.parents[1]/'DesktopSnow-macOS-v2.9-Metal-source.zip'
if baseline.exists():
    with zipfile.ZipFile(baseline) as z:
        for path in ['Sources/Particles.metal','Sources/MetalParticles.swift','Core/MonitorPolicy.h']:
            check((root/path).read_bytes() == z.read('DesktopSnow/'+path), 'unchanged v2.9 hybrid GPU/monitor production '+path)
else:
    print('SKIP baseline ZIP byte-comparison (optional beside workdir)')
print('NOT VERIFIED: Swift macOS type-check, MSL compilation, drawable presentation, native visuals/performance. Run --metal-self-test on Mac.')
