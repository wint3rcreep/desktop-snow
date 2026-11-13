from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'Sources/main.swift').read_text();h=(r/'Core/Snow.h').read_text();c=(r/'Core/Snow.cpp').read_text()
actor=s[s.index('    func drawTractor'):s.index('    func drawAccessories')]
assert 'snow_tractor_pose(t.x,t.y)' in actor
for token in ['let angle=t.angle*facing','sqrt(t.cargo)*0.24','max(0.16,abs(pose.heading))','y:-pose.lift','by:pose.pitch','alpha:0.08']:
 assert token in actor,token
assert 'Date(' not in actor and 'CACurrentMediaTime' not in actor and 'setShadow' not in actor
assert s.count('for t in c.tractors { drawTractor(t, context) }')==1
assert 'typedef struct { double x,y,direction,angle,cargo; int active; } SnowTractor;' in h
assert 'SnowTractorPose snow_tractor_pose(double x,double y);' in h
assert 'cycle.hauledArea+=t.cargo;t.cargo=0;' in c
assert 'cycles[k]' in c and 'if(cycle.passes>=2||fullness<=.2)' in c
print('PASS shared AppKit tractor read-only pose actual-radius wheel/mound/subtle-lamp ABI contracts (not native visual verification)')
