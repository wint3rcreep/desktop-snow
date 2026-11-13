from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'Sources/main.swift').read_text();h=(r/'Core/Snow.h').read_text()
for token in ['snow_ball_pose(s.x,s.y)','1-pose.squash','pose.tilt','s.angle + pose.tilt','pose.flutter','let smile=NSBezierPath()','scarfShade']:
    assert token in s,token
assert 'typedef struct { double x,y,r,alpha; int kind; double angle; } SnowSprite;' in h
assert 'typedef struct { double squash,tilt,flutter; } SnowBallPose;' in h
print('PASS cosmetic Swift pose/face/accessory contracts; unchanged SnowSprite ABI (not native type-check)')
