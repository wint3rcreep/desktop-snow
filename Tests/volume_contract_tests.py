from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'Sources/main.swift').read_text()
assert 'drawVolumetricBank(path' in s, 'production bank must use independent volumetric renderer'
body=s.split('private func drawVolumetricBank')[1].split('private func')[0]
assert 'context.clip()' in body
assert body.count('CGGradient(')>=3, 'independent blue body, white crown, contact shadow'
assert 'setShadow' not in body, 'no external shadow overhang at C1 caps'
assert 'snow_windows_at($0.baseAddress' in s and 'systemUptime' in s
print('PASS clipped three-layer vector volume and actual timestamp production dispatch contracts (not visual runtime)')
