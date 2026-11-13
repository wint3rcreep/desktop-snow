#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { double x,y,w,h; } SnowRect;
typedef struct { long long id; SnowRect rect; } SnowWindow;
// kind: 0 flake, 1 ordinary ball, 2 complete-assembly head, 3 torso, 4 base, 5 loose airborne clump.
// Roles are recomputed from live parent links, never permanent decorations.
typedef struct { double x,y,r,alpha; int kind; double angle; } SnowSprite;
typedef struct { double x,y,w,h; } SnowPile;
// Assumed rounded-window top inset in points; 10 default, 0 for square windows.
// Set before the next snow_windows snapshot. Changing support releases old snow.
void snow_corner_inset(double points);
// Cosmetic only: no collision/radius/mass changes. Zero pose for non-balls.
// Separate ABI leaves SnowSprite and MetalParticle layouts unchanged.
typedef struct { double squash,tilt,flutter; } SnowBallPose;
SnowBallPose snow_ball_pose(double x,double y);
// Cubic Bezier top segments. start/end explicitly delimit independent closed banks.
// Close each bank along baseY; there are no vertical endcaps (endpoint is on base).
typedef struct {double x0,y0,x1,y1,x2,y2,x3,y3,baseY; int start,end;} SnowCurve;
// Returns REQUIRED count, even if cap is smaller. Query with NULL,0 before export.
int snow_contours(SnowCurve *out,int cap);
double snow_window_interval(int lowPower,int pointerDown,int recentMotion);
// One independent snowplow per display; x/y is body centre at floor level.
typedef struct { double x,y,direction,angle,cargo; int active; } SnowTractor;
// Read-only simulation-time pose; wheel radius is exactly 10 points.
// Cargo is already t.cargo, mound is only its visual proxy, never added mass.
typedef struct { double speed,heading,lift,pitch,turnRemaining,hauledArea; } SnowTractorPose;
SnowTractorPose snow_tractor_pose(double x,double y);
void snow_tractor_enabled(int enabled);
int snow_tractors(SnowTractor *out,int cap);
void snow_reset(void);
// Space/wake/display transition: discard supports without a shower.
void snow_forget_windows(void);
// Disabled physical rectangles remove airborne/interactive material only there.
// Submit before snow_displays; absent/new native display IDs default enabled.
void snow_disabled_displays(const SnowRect *rects, int count);
void snow_displays(const SnowRect *rects, int count);
void snow_windows(const SnowWindow *windows, int count);
// Monotonic actual CGWindow snapshot time; history is keyed by stable window ID.
void snow_windows_at(const SnowWindow *windows, int count, double timestamp);
void snow_step(double dt, double intensity, double wind);
int snow_sprites(SnowSprite *out, int cap);
// Piles include short-lived detached patches, released at the OLD support then freely falling.
int snow_piles(SnowPile *out, int cap);
// Area is 2D material accounting, not real snow volume. Excludes fresh flakes/balls.
typedef struct { double pileArea,airborneArea,pendingArea; int grains,bands,shedLastStep; } SnowDiagnostics;
SnowDiagnostics snow_diagnostics(void);
void snow_down(double x,double y,int sweep);
void snow_drag(double x,double y,int sweep);
void snow_drag_timed(double x,double y,int sweep,double dt);
void snow_up(void);
#ifdef __cplusplus
}
#endif
