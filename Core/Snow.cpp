#include "Snow.h"
#include "MonitorPolicy.h"
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <random>
#include <cstdint>
namespace {
constexpr double bin=8, pi=3.141592653589793;
struct Surface { SnowRect r; std::vector<double> h; std::vector<bool> visible; double sampled=0,vx=0; double rawX=0,rawY=0,rawTime=0,rawVx=0,rawVy=0,acceleration=0,releaseSeconds=.18; bool motionReady=false,velocityReady=false; std::vector<size_t> avalanche={}; size_t avalancheCursor=1; double avalancheCredit=0; double edgeAge[2]={0,0},edgeWait[2]={0,0}; };
struct Particle { double x,y,r,v,phase,vx=0,vy=0; };
struct Ball { double x,y,r,vx=0,vy=0,angle=0; int parent=-1; bool face=false; double squash=0,squashV=0,tilt=0,tiltV=0,flutter=0,flutterV=0; };
std::map<long long,Surface> surfaces;
std::vector<SnowRect> displays;
std::vector<SnowRect> disabledDisplays;
std::vector<SnowWindow> windows;
std::vector<Particle> flakes;
std::vector<SnowTractor> tractors;
bool tractorEnabled=false;
struct PlowCycle {
 int passes=0;bool armed=true;
 double speed=0,heading=1,lift=0,pitch=0,turnRemaining=0,hauledArea=0;
};
std::vector<PlowCycle> cycles;
std::vector<Ball> balls;
struct Clump { double x,y,r,vy,area; long long source; double vx=0,angle=0,spin=0; };
std::vector<Clump> clumps;
constexpr size_t maxClumps=2048, maxShedding=4096;
constexpr size_t maxReleasePerStep=96;
// Detached patches retain old-world release positions, then fall and fracture; never live supports.
struct Shedding { double x,bottom,width,area,rate,delay,vx; long long source; unsigned serial=0; double depth=0,credit=0,target=0,vy=0; };
std::vector<Shedding> shedding;
double deposit(Surface &,size_t,double);
double contourTop(const Surface &,size_t,double);
double patchTop(const Shedding &b,double x){
 Surface s{{b.x,b.bottom,b.width,1},{b.depth>0?std::min(b.depth,b.area/b.width):b.area/b.width},{true},0,0};
 return contourTop(s,0,x);
}
double cornerInset=10;
// Separate reproducible stream: unrelated falling-flake RNG never synchronizes mouths.
uint32_t grainSeed=0x52a9d317u;
double grainUnit(){grainSeed^=grainSeed<<13;grainSeed^=grainSeed>>17;grainSeed^=grainSeed<<5;return double(grainSeed)/4294967296.0;}
double grainRange(double lo,double hi){return lo+(hi-lo)*grainUnit();}
Clump loose(double x,double y,double area,long long source,double vx){
 return {x,y,std::sqrt(area/pi),grainRange(-5,7),area,source,
         std::clamp(vx+grainRange(-4,4),-120.0,120.0),grainRange(0,2*pi),grainRange(-3,3)};
}
double simulationTime=0;
double snapshotTime=0; bool explicitSnapshot=false;
int shedLastStep=0;
void release(long long id,const Surface &s,size_t first=0,double vx=0){
 for(size_t i=first;i<s.h.size();++i){
  double width=std::min(bin,s.r.w-i*bin);
  if(s.h[i]<=0||width<=0){continue;}
  double a=width*s.h[i];
  Shedding band{s.r.x+i*bin,s.r.y,width,a,a/s.releaseSeconds,grainRange(.008,.065),vx,id};
  if(shedding.size()<maxShedding){shedding.push_back(band);}
  else{
   // Extreme pressure: coalesce material, never silently discard released area.
   // Prefer the same old anchor. Spatial detail degrades only at this hard cap.
   auto best=shedding.begin();double distance=1e30;
   for(auto it=shedding.begin();it!=shedding.end();++it){
    double d=std::abs(it->x-band.x)+std::abs(it->bottom-band.bottom);
    if(d<distance){distance=d;best=it;}
   }
   best->area+=a;best->rate=std::max(best->rate,best->area/.18);
  }
 }
}
void shed(double dt){
 size_t emitted=0;shedLastStep=0;
 for(auto it=shedding.begin();it!=shedding.end();){
  // A detached patch is ballistic from release, including during fracture delay.
  double oldBottom=it->bottom;
  double decay=std::exp(-1.5*dt);it->x+=it->vx*(1-decay)/1.5;it->vx*=decay;
  it->bottom+=it->vy*dt+230*dt*dt;it->vy+=460*dt;
  Surface *catcher=nullptr;size_t catchBin=0;double catchY=1e30;
  double cx=it->x+it->width/2;
  for(auto &e:surfaces){auto &s=e.second;if(e.first==it->source||cx<s.r.x||cx>=s.r.x+s.r.w)continue;
   size_t j=(size_t)((cx-s.r.x)/bin);if(j<s.h.size()&&s.visible[j]&&oldBottom<=s.r.y+.001&&it->bottom>=s.r.y&&s.r.y<catchY){catcher=&s;catchBin=j;catchY=s.r.y;}}
  if(catcher){it->area=deposit(*catcher,catchBin,it->area);}
  double active=std::max(0.0,dt-it->delay);it->delay=std::max(0.0,it->delay-dt);
  it->credit=std::min(it->area,it->credit+it->rate*active);
  if(it->target==0)it->target=grainRange(16,32);
  while(it->area>1e-9&&it->credit+1e-9>=std::min(it->area,it->target)&&clumps.size()<maxClumps&&emitted<maxReleasePerStep){
   // Bounded seeded fracture sizes/positions; all grains retain exact material area.
   double portion=std::min(it->area,it->target);
   double r=std::sqrt(portion/pi),fraction=grainRange(.04,.96);
   double x=it->x+it->width*fraction,y=patchTop(*it,x)+r;
   auto grain=loose(x,y,portion,it->source,it->vx);grain.vy+=it->vy;clumps.push_back(grain);
   it->area-=portion;it->credit=std::max(0.0,it->credit-portion);it->target=grainRange(16,32);++it->serial;++emitted;++shedLastStep;
  }
  if(it->area<=1e-9){it=shedding.erase(it);}else{++it;}
 }
}
double deposit(Surface &s,size_t center,double area){
 size_t left=center,right=center;
 while(left>0&&s.visible[left-1])--left;
 while(right+1<s.h.size()&&s.visible[right+1])++right;
 for(size_t distance=0;distance<s.h.size()&&area>0;++distance){
  for(int side=0;side<(distance==0?1:2);++side){
   long long j=(long long)center+(side==0?(long long)distance:-(long long)distance);
   if(j<(long long)left||j>(long long)right||!s.visible[j]){continue;}
   double width=std::min(bin,s.r.w-j*bin);
   if(width<=0){continue;}
   double take=std::min(area,std::max(0.0,42-s.h[j])*width);
   s.h[j]+=take/width;area-=take;
  }
 }
 return area;
}
// Route only outside an exposed physical edge, never through the window body.
bool overflow(long long id,const Surface &s,double area,Clump &out,size_t center){
 if(id<0||area<=0)return false;
 bool left=true,right=true;
 for(size_t i=0;i<=center;++i)left=left&&s.visible[i];
 for(size_t i=center;i<s.h.size();++i)right=right&&s.visible[i];
 if(!left&&!right)return false;
 int side=left?0:1;
 if(left&&right)side=center<s.h.size()/2?0:1;
 // Queue overflow at the ACTUAL physical edge, not across the last bin.
 // Reservoir pressure returns false: caller keeps the unaccepted parcel.
 if(area>32){
  if(shedding.size()>=maxShedding)return false;
  double x=side==0?s.r.x-4:s.r.x+s.r.w;
  double rest=area-32;
  shedding.push_back({x,s.r.y,4,rest,rest/.18,0,side==0?-15.0:15.0,id,0,4});
  area=32;
 }
 double r=std::sqrt(area/pi);
 out=loose(side==0?s.r.x-r-.01:s.r.x+s.r.w+r+.01,s.r.y-r,area,id,side==0?-15.0:15.0);return true;
}
// Each visible nonempty run has its own area-normalized C1 contour.
// Smoothstep Beziers have horizontal end tangents and convex-hull bounds:
// no Catmull-Rom overshoot, vertical end wall, or cross-run bridge.
struct Knot {double x,h;};
std::vector<Knot> contour(const Surface &s,size_t center){
 if(center>=s.h.size()||!s.visible[center]||s.h[center]<=0)return {};
 size_t l=center,r=center;
 while(l>0&&s.visible[l-1]&&s.h[l-1]>0)--l;
 while(r+1<s.h.size()&&s.visible[r+1]&&s.h[r+1]>0)++r;
 std::vector<Knot> k{{s.r.x+l*bin,0}};double area=0;
 for(size_t i=l;i<=r;++i){double w=std::min(bin,s.r.w-i*bin);
  k.push_back({s.r.x+i*bin+w/2,s.h[i]});area+=s.h[i]*w;
 }
 k.push_back({s.r.x+std::min(s.r.w,(r+1)*bin),0});
 // A cap must occupy a real ramp, not compress a deep bank into 4 points.
 // Shape the supported run before normalization; removed shoulder area is
 // redistributed through this SAME run, never dropped or moved across a gap.
 double peak=0;for(auto v:k)peak=std::max(peak,v.h);
 double ramp=std::min((k.back().x-k.front().x)/2,std::clamp(peak*.65,8.0,24.0));
 for(auto &v:k){
  double t=std::clamp(std::min(v.x-k.front().x,k.back().x-v.x)/ramp,0.0,1.0);
  v.h*=t*t*(3-2*t);
 }
 double integral=0;for(size_t i=1;i<k.size();++i)integral+=(k[i].x-k[i-1].x)*(k[i].h+k[i-1].h)/2;
 // Exact integral of smoothstep is 1/2. Scaling preserves all accepted area.
 if(integral>0)for(auto &v:k)v.h*=area/integral;
 return k;
}
double contourTop(const Surface &s,size_t j,double x){
 auto k=contour(s,j);if(k.empty())return s.r.y;
 for(size_t i=1;i<k.size();++i)if(x<=k[i].x){
  double t=std::clamp((x-k[i-1].x)/(k[i].x-k[i-1].x),0.0,1.0);
  return s.r.y-(k[i-1].h+(k[i].h-k[i-1].h)*t*t*(3-2*t));
 }
 return s.r.y;
}
std::mt19937 rng(9271);
double credit=0,lastX=0,lastY=0; int held=-1;
// Resolve parent-first even when balls were created in a different order.
void follow(int i, int depth=0){
 if(depth>3||balls[i].r<=0)return;
 auto &b=balls[i];if(b.parent<0||b.parent>=(int)balls.size()||balls[b.parent].r<=0)return;
 follow(b.parent,depth+1);auto &p=balls[b.parent];b.x=p.x;b.y=p.y-p.r-b.r+2;
}
void syncAssembly(){
 for(auto &b:balls)b.face=false;
 for(int i=0;i<(int)balls.size();++i){follow(i);auto &b=balls[i];
  if(b.r>0&&b.parent>=0&&balls[b.parent].r>0){int base=balls[b.parent].parent;
   if(base>=0&&balls[base].r>0)b.face=true;}
 }
}
int ballKind(int i){
 if(balls[i].face)return 2;
 for (const auto &head : balls) {
  if (head.r > 0 && head.face) {
   if (head.parent == i) return 3;
   if (balls[head.parent].parent == i) return 4;
  }
 }
 return 1;
}
double random(double a,double b){return std::uniform_real_distribution<double>(a,b)(rng);}
void surface(long long id,SnowRect r,const std::vector<SnowRect>& front){
 auto &s=surfaces[id];
 // Actual snapshot samples are separate from the accepted pile anchor. Differencing
 // filtered velocity would create fictitious acceleration during constant-speed drag.
 double now=explicitSnapshot?snapshotTime:simulationTime;
 double elapsed=now-s.rawTime,dx=r.x-s.rawX,dy=r.y-s.rawY;
 bool valid=s.motionReady&&std::isfinite(elapsed)&&elapsed>=1.0/240&&elapsed<=.5&&std::hypot(dx,dy)<=200;
 if(valid){
  double ux=std::clamp(dx/elapsed,-1600.0,1600.0),uy=std::clamp(dy/elapsed,-1600.0,1600.0);
  if(std::hypot(dx,dy)<=.5){ux=0;uy=0;}
  double a=s.velocityReady?std::min(8000.0,std::hypot(ux-s.rawVx,uy-s.rawVy)/elapsed):0;
  double blend=1-std::exp(-elapsed/.06);
  s.acceleration+=(a-s.acceleration)*blend;
  s.vx+=(std::clamp(ux*.18,-120.0,120.0)-s.vx)*blend;
  s.rawVx=ux;s.rawVy=uy;s.velocityReady=true;
  double drive=std::clamp(std::hypot(ux,uy)/500+s.acceleration/4000,0.0,1.0);
  s.releaseSeconds=.70-.58*drive;
 }else{
  s.vx=0;s.acceleration=0;s.velocityReady=false;s.releaseSeconds=.18;
 }
 s.rawX=r.x;s.rawY=r.y;s.rawTime=now;s.motionReady=true;
 bool moved=s.r.w>0&&(std::abs(s.r.x-r.x)>.5||std::abs(s.r.y-r.y)>.5);
 if(moved){
  // Every unsupported area detaches at the OLD world anchor, even on a slow drag.
  // Motion changes breakup rate and modest horizontal drift; detached patches fall independently.
  release(id,s,0,s.vx);s.h.clear();s.avalanche.clear();s.avalancheCredit=0;
 }
 else if(s.r.w>0){
  s.vx*=std::exp(-6*std::max(0.0,simulationTime-s.sampled));
  // Retain the accepted anchor so slow movement eventually crosses tolerance.
  r.x=s.r.x;r.y=s.r.y;
  if(r.w>s.r.w&&!s.h.empty()){
   // A formerly partial last bin gains support, not snow. Preserve its area.
   size_t last=s.h.size()-1;
   s.h[last]*=std::min(bin,s.r.w-last*bin)/std::min(bin,r.w-last*bin);
  }
  if(r.w<s.r.w){
   size_t keep=(size_t)std::max(0.0,std::floor(r.w/bin));
   release(id,s,keep);
   if(keep<s.h.size()){s.h.resize(keep);}
  }
 }
 s.sampled=simulationTime;
 s.r=r;
 s.h.resize(std::max(1,(int)std::ceil(r.w/bin)),0); s.visible.assign(s.h.size(),true);
 for(size_t i=0;i<s.h.size();++i) for(auto f:front)
  if(r.y+.5>=f.y&&r.y+.5<f.y+f.h&&r.x+i*bin<f.x+f.w&&r.x+std::min(r.w,(i+1)*bin)>f.x){s.visible[i]=false;break;}
}
void scatter(double x,double bottom,double area,double vx,double vy){
 if(area<=0||clumps.size()>=maxClumps){return;}
 double r=std::sqrt(area/pi);
 // No excluded support: broom snow may land back on the very same window/floor.
 clumps.push_back({x,bottom-r,r,vy,area,(-9223372036854775807LL-1),vx});
}
// One smooth impulse per event, not per interpolated pile sample. No allocations.
void sweepAir(double ax,double ay,double bx,double by,double vx,double vy){
 double dx=bx-ax,dy=by-ay,length2=dx*dx+dy*dy;
 auto impulse=[&](double x,double y,double &ux,double &uy){
  double t=length2>0?std::clamp(((x-ax)*dx+(y-ay)*dy)/length2,0.0,1.0):0;
  double rx=x-(ax+t*dx),ry=y-(ay+t*dy),distance=std::hypot(rx,ry);
  if(distance>=45){return;}
  double weight=1-distance/45;weight=weight*weight*(3-2*weight);
  // Fade radial force at the centre as well as the boundary; no singular direction.
  double nx=rx/std::max(8.0,distance),ny=ry/std::max(8.0,distance);
  double tx=std::clamp(vx+nx*120,-500.0,500.0);
  double ty=std::clamp(vy+ny*120,-490.0,260.0);
  ux+=(tx-ux)*weight;uy+=(ty-uy)*weight;
 };
 for(auto &p:flakes){impulse(p.x,p.y,p.vx,p.vy);}
 for(auto &p:clumps){impulse(p.x,p.y,p.vx,p.vy);}
}
void sweepAt(double x,double y,double vx=0,double vy=-140){
 for(auto &entry:surfaces){
  auto &s=entry.second;
  for(size_t i=0;i<s.h.size();++i){
   double cx=s.r.x+i*bin+std::min(bin,s.r.w-i*bin)/2;
   if(s.visible[i]&&s.h[i]>0&&clumps.size()<maxClumps&&std::hypot(cx-x,s.r.y-y)<45){
    double width=std::min(bin,s.r.w-i*bin);
    scatter(cx,s.r.y-s.h[i],s.h[i]*width,vx,vy);
    s.h[i]=0;
   }
  }
 }
 for(int i=(int)balls.size()-1;i>=0;--i){
  if(balls[i].r>0&&clumps.size()<maxClumps&&std::hypot(balls[i].x-x,balls[i].y-y)<45+balls[i].r){
   auto &b=balls[i];scatter(b.x,b.y+b.r,pi*b.r*b.r,vx,vy);
   // Broken snowman becomes airborne snow; untouched children safely detach.
   b.r=0;b.face=false;b.parent=-1;
   for(auto &child:balls){if(child.parent==i){child.parent=-1;}}
  }
 }
}
double collect(double x,double y,double radius,double budget=1e20){double area=0;
 for(auto &entry:surfaces){auto &s=entry.second; for(size_t i=0;i<s.h.size();++i)
  if(s.visible[i]&&std::abs(s.r.x+(i+.5)*bin-x)<radius&&std::abs(s.r.y-y)<radius+20){
   double width=std::min(bin,s.r.w-i*bin);double take=std::min({s.h[i],3.0,std::max(0.0,budget-area)/width});s.h[i]-=take;area+=take*width;
  }}return area;
}
}
extern "C" {
SnowBallPose snow_ball_pose(double x,double y){
 syncAssembly();
 for(int i=0;i<(int)balls.size();++i){const auto &b=balls[i];
  if(b.r<=0||b.x!=x||b.y!=y)continue;
  int root=i;for(int depth=0;depth<3&&balls[root].parent>=0;++depth)root=balls[root].parent;
  const auto &a=balls[root];bool assembled=ballKind(i)>=2;
  return {assembled?0:b.squash,assembled?a.tilt:0,assembled?a.flutter:0};
 }return {0,0,0};
}
void snow_corner_inset(double points){if(std::isfinite(points))cornerInset=std::clamp(points,0.0,40.0);}
int snow_contours(SnowCurve *out,int cap){
 int n=0;
 auto emit=[&](const Surface &s,size_t j){
  auto k=contour(s,j);
  for(size_t i=1;i<k.size();++i){auto a=k[i-1],b=k[i];double dx=(b.x-a.x)/3;
   SnowCurve c{a.x,s.r.y-a.h,a.x+dx,s.r.y-a.h,b.x-dx,s.r.y-b.h,b.x,s.r.y-b.h,s.r.y,i==1?1:0,i+1==k.size()?1:0};
   if(out&&n<cap)out[n]=c;
   ++n;
  }
 };
 for(const auto &e:surfaces){const auto &s=e.second;
  for(size_t i=0;i<s.h.size();){
   if(!s.visible[i]||s.h[i]<=0){++i;continue;}
   emit(s,i);do{++i;}while(i<s.h.size()&&s.visible[i]&&s.h[i]>0);
  }
 }
 // Detached material is not a live support. Each reservoir is independently
 // capped: it must never stitch together unrelated windows or occlusion gaps.
 for(const auto &b:shedding)if(b.area>0){Surface s{{b.x,b.bottom,b.width,1},{b.depth>0?std::min(b.depth,b.area/b.width):b.area/b.width},{true},0,0};emit(s,0);}
 return n;
}
double snow_window_interval(int lowPower,int pointerDown,int recentMotion){return lowPower ? 0.5 : ((pointerDown||recentMotion) ? 1.0/30.0 : 0.12);}
void snow_reset(){for(auto &e:surfaces){e.second.avalanche.clear();e.second.avalancheCredit=0;e.second.avalancheCursor=1;e.second.motionReady=false;e.second.velocityReady=false;e.second.acceleration=0;e.second.vx=0;e.second.releaseSeconds=.18;}grainSeed=0x52a9d317u;for(auto &e:surfaces){for(int side=0;side<2;++side){e.second.edgeAge[side]=0;e.second.edgeWait[side]=0;}}flakes.clear();clumps.clear();shedding.clear();shedLastStep=0;balls.clear();held=-1;credit=0;for(auto &c:cycles)c={};for(auto &t:tractors){t.cargo=0;t.active=0;}for(auto &e:surfaces)std::fill(e.second.h.begin(),e.second.h.end(),0);}
void snow_forget_windows(){surfaces.clear();windows.clear();clumps.clear();shedding.clear();shedLastStep=0;for(auto &t:tractors){t.cargo=0;t.active=0;}}
void snow_tractor_enabled(int enabled){
 if(tractorEnabled!=(enabled!=0)){
  for(auto &c:cycles)c={};
  for(auto &t:tractors){t.active=0;t.cargo=0;}
 }
 tractorEnabled=enabled!=0;
}
SnowTractorPose snow_tractor_pose(double x,double y){
 for(size_t k=0;k<tractors.size();++k)if(tractors[k].x==x&&tractors[k].y==y){
  const auto &c=cycles[k];return {c.speed,c.heading,c.lift,c.pitch,c.turnRemaining,c.hauledArea};
 }
 return {};
}
int snow_tractors(SnowTractor *out,int cap){
 int n=0;if(!tractorEnabled){return 0;}
 for(auto t:tractors){if(t.active&&n<cap){out[n++]=t;}}return n;
}
// Changing monitor policy is a local discard, not a global reset or shower.
void monitorPrune(bool dynamicOnly=false){
 if(dynamicOnly&&disabledDisplays.empty())return;
 if(!dynamicOnly)for(auto &e:surfaces){auto &s=e.second;for(size_t i=0;i<s.h.size();++i)
  if(displays.empty()||monitorBlockedShelf(disabledDisplays,s.r.x+i*bin,s.r.y,std::min(bin,s.r.w-i*bin))){s.h[i]=0;s.visible[i]=false;}}
 flakes.erase(std::remove_if(flakes.begin(),flakes.end(),[](const auto &p){return (displays.empty()||monitorPoint(disabledDisplays,p.x,p.y));}),flakes.end());
 clumps.erase(std::remove_if(clumps.begin(),clumps.end(),[](const auto &p){return (displays.empty()||monitorPoint(disabledDisplays,p.x,p.y));}),clumps.end());
 shedding.erase(std::remove_if(shedding.begin(),shedding.end(),[](const auto &p){return displays.empty()||monitorBlockedShelf(disabledDisplays,p.x,p.bottom,p.width);}),shedding.end());
 for(auto &b:balls)if(displays.empty()||monitorPoint(disabledDisplays,b.x,b.y))b.r=0;
 for(auto &b:balls)if(b.parent>=0&&balls[b.parent].r<=0)b.parent=-1;
 if(held>=0&&balls[held].r<=0)held=-1;
}
void snow_disabled_displays(const SnowRect *r,int n){disabledDisplays.clear();if(n>0)disabledDisplays.assign(r,r+n);}
void snow_displays(const SnowRect *r,int n){
 auto oldDisplays=displays;auto oldTractors=tractors;auto oldCycles=cycles;
 std::map<long long,Surface> floors;
 for(size_t i=0;i<oldDisplays.size();++i){auto it=surfaces.find(-1-(long long)i);if(it!=surfaces.end()){floors.emplace(it->first,std::move(it->second));surfaces.erase(it);}}
 displays.clear();tractors.clear();cycles.clear();if(n>0)displays.assign(r,r+n);
 for(size_t i=0;i<displays.size();++i){auto d=displays[i];
  cycles.push_back({});tractors.push_back({d.x+std::min(40.0,d.w/2),d.y+d.h-1,1,0,0,0});
  for(size_t j=0;j<oldDisplays.size();++j)if(monitorSame(d,oldDisplays[j])){
   tractors[i]=oldTractors[j];cycles[i]=oldCycles[j];auto f=floors.find(-1-(long long)j);
   if(f!=floors.end()){surfaces.emplace(-1-(long long)i,std::move(f->second));}
   break;
  }
 }
 held=-1;monitorPrune();
}
void snow_windows_at(const SnowWindow *w,int n,double timestamp){
 explicitSnapshot=std::isfinite(timestamp);snapshotTime=timestamp;
 snow_windows(w,n);explicitSnapshot=false;
}
void snow_windows(const SnowWindow *w,int n){
 windows.clear();if(n>0)windows.assign(w,w+n);
 std::vector<long long> live;std::vector<SnowRect> front;
 for(auto window:windows){
  auto support=window.rect;
  // CGWindow supplies bounds, NOT a corner radius. Conservative flat top shelf.
  double inset=std::min(cornerInset,std::max(0.0,(support.w-1)/2));
  support.x+=inset;support.w-=2*inset;
  surface(window.id,support,front);live.push_back(window.id);front.push_back(window.rect);
 }
 for(size_t i=0;i<displays.size();++i){auto r=displays[i];r.y+=r.h-1;r.h=1;long long id=-1-(long long)i;
  surface(id,r,{});live.push_back(id);}
 for(auto it=surfaces.begin();it!=surfaces.end();){
  if(std::find(live.begin(),live.end(),it->first)==live.end()){
   if(it->first>=0){release(it->first,it->second);}
   it=surfaces.erase(it);
  }else{++it;}
 }
 monitorPrune();
}
void snow_step(double dt,double intensity,double wind){
 if(displays.empty())return;
 monitorPrune(true);
 if(!std::isfinite(dt))return;
 dt=std::clamp(dt,0.0,0.05);if(dt==0)return;
 simulationTime+=dt;shed(dt);
 // Fixed bounded substeps, damped springs, no wall-clock/random animation.
 // Advance once per simulation step; exports/redraws never advance time.
 auto spring=[](double &q,double &v,double target,double limit,double h){
  v+=(100*(target-q)-16*v)*h;q+=v*h;
  if(std::abs(q)>limit){q=std::clamp(q,-limit,limit);v=0;}
  if(std::abs(q-target)<1e-8&&std::abs(v)<1e-8){q=target;v=0;}
 };
 int sub=std::max(1,(int)std::ceil(dt/.008));double h=dt/sub;
 double breeze=std::clamp(std::isfinite(wind)?wind:0.0,-100.0,100.0);
 for(auto &b:balls)for(int k=0;k<sub;++k){
  spring(b.squash,b.squashV,0,.12,h);
  spring(b.tilt,b.tiltV,breeze*.00018,.055,h);
  spring(b.flutter,b.flutterV,breeze*.0006,.10,h);
 }
 for(auto d:displays){credit+=dt*std::clamp(intensity,0.0,3.0)*d.w/18;
  while(credit>=1&&flakes.size()<7000){credit-=1;flakes.push_back({random(d.x,d.x+d.w),d.y-5,random(.9,3.0),random(24,80),random(0,6.28)});}}
 credit=std::min(credit,1.0);
 for(auto it=flakes.begin();it!=flakes.end();){auto &p=*it;double old=p.y;p.phase+=dt;
  // Transient velocity decays back to natural terminal snowfall (including after upward throws).
  double decay=std::exp(-2.5*dt),travel=(1-decay)/2.5;
  p.x+=(wind+std::sin(p.phase*1.8)*13)*dt+p.vx*travel;
  p.y+=p.v*dt+p.vy*travel;p.vx*=decay;p.vy*=decay;bool hit=false;
  Surface *best=nullptr;size_t index=0;double top=1e20;
  for(auto &e:surfaces){auto &s=e.second;if(p.x<s.r.x||p.x>=s.r.x+s.r.w)continue;size_t j=(size_t)((p.x-s.r.x)/bin);if(j>=s.h.size()||!s.visible[j])continue;
   double y=contourTop(s,j,p.x);if(p.v+p.vy>=0&&old<=y&&p.y+p.r>=y&&y<top){best=&s;index=j;top=y;}}
  if(best){
   double remaining=deposit(*best,index,p.r*.5*bin);hit=true;
   if(remaining>1e-9){
    long long id=0;for(const auto &e:surfaces)if(&e.second==best)id=e.first;
    Clump q{};
    if(clumps.size()<maxClumps&&overflow(id,*best,remaining,q,index))clumps.push_back(q);
    else if(id>=0){p.r=remaining/(.5*bin);p.y=top-p.r;hit=false;} // bounded backpressure, not deletion
    // A full finite floor is a terminal sink (42 points), not another window.
   }
  }
  bool on=false;for(auto d:displays)if(p.y<=d.y+d.h+100&&p.y>=d.y-400&&p.x>=d.x-400&&p.x<=d.x+d.w+400)on=true;
  if(hit||!on)it=flakes.erase(it);else ++it;
 }
 // Swept collisions prevent tunneling; source ID is excluded even during downward drags.
 for(auto it=clumps.begin();it!=clumps.end();){
  auto &p=*it;double old=p.y+p.r;
  double decay=std::exp(-1.5*dt);p.x+=p.vx*(1-decay)/1.5;p.vx*=decay;
  p.angle+=p.spin*dt;
  p.y+=p.vy*dt+230*dt*dt;p.vy+=460*dt;
  Surface *best=nullptr;long long bestId=0;size_t index=0;double top=1e20;
  for(auto &e:surfaces){
   if(e.first==p.source){continue;}
   auto &s=e.second;
   if(p.x<s.r.x||p.x>=s.r.x+s.r.w){continue;}
   size_t j=(size_t)((p.x-s.r.x)/bin);
   if(j>=s.h.size()||!s.visible[j]){continue;}
   // A bank can grow upward between grains. Catch penetration above the
   // physical base too, otherwise dense streams tunnel through the rising pile.
   double y=contourTop(s,j,p.x);
   if(p.vy>=0&&old<=s.r.y+.001&&p.y+p.r>=y&&y<top){best=&s;bestId=e.first;index=j;top=y;}
  }
  double maxY=-1e20;for(auto d:displays){maxY=std::max(maxY,d.y+d.h);}
  if(best){
   double remaining=deposit(*best,index,p.area);Clump q{};
   if(remaining>1e-9&&bestId>=0){
    if(overflow(bestId,*best,remaining,q,index)){p=q;}
    else{p.area=remaining;p.r=std::sqrt(remaining/pi);p.y=top-p.r;p.vy=0;}
    ++it;
   }else{it=clumps.erase(it);}
  }
  else if(p.y-p.r>maxY+100){it=clumps.erase(it);}
  else{++it;}
 }
 // Autonomous plows touch only their own floor bins, never windows or airborne snow.
 if(tractorEnabled){
  for(size_t k=0;k<tractors.size();++k){
   auto &t=tractors[k];auto d=displays[k];double margin=std::min(40.0,d.w/2);
   double left=d.x+margin,right=d.x+d.w-margin;
   auto entry=surfaces.find(-1-(long long)k);
   if(entry==surfaces.end()){continue;}
   auto &s=entry->second;double groundArea=0;
   for(size_t i=0;i<s.h.size();++i){groundArea+=s.h[i]*std::min(bin,s.r.w-i*bin);}
   double fullness=groundArea/(42*std::max(1.0,s.r.w));
   auto &cycle=cycles[k];
   if(!t.active&&fullness<.6){cycle.armed=true;}
   if(!t.active&&cycle.armed&&fullness>.8+1e-12){t.active=1;cycle.passes=0;cycle.armed=false;cycle.heading=t.direction;cycle.speed=0;cycle.turnRemaining=0;}
   if(t.active&&fullness<=.2&&t.cargo==0){t.active=0;cycle.speed=0;cycle.lift=0;cycle.pitch=0;cycle.turnRemaining=0;}
   if(!t.active){continue;}
   // A stationary .45s yaw at the endpoint is NOT an additional pass.
   if(cycle.turnRemaining>0){
    cycle.turnRemaining=std::max(0.0,cycle.turnRemaining-dt);
    double u=1-cycle.turnRemaining/.45;
    cycle.heading=t.direction*(2*u*u*(3-2*u)-1);
    continue;
   }
   cycle.heading=t.direction;
   double remaining=std::max(0.0,t.direction>0?right-t.x:t.x-left);
   constexpr double acceleration=140, braking=180;
   double previous=cycle.speed;
   double travel=0;
   if(previous<=braking*dt&&remaining<=previous*dt*.5+1e-9){
    // Finish a braking segment inside this timestep, not a frame-late speed snap.
    cycle.speed=0;travel=remaining;
   }else{
    // Trapezoidal travel + next speed's stopping distance must fit remaining.
    double discriminant=braking*braking*dt*dt+8*braking*remaining-4*braking*previous*dt;
    double safe=std::max(0.0,(std::sqrt(std::max(0.0,discriminant))-braking*dt)*.5);
    cycle.speed=std::min({70.0,previous+acceleration*dt,safe});
    travel=std::min(remaining,(previous+cycle.speed)*.5*dt);
   }
   double oldX=t.x;t.x+=t.direction*travel;
   t.angle=std::remainder(t.angle+(t.x-oldX)/10,2*pi);
   // Ground-following, overdamped bounded cosmetic suspension, no rest excitation.
   auto height=[&](double x){size_t i=(size_t)std::clamp((x-s.r.x)/bin,0.0,double(s.h.size()-1));return s.h[i];};
   double rear=height(t.x-17),front=height(t.x+16);
   double blend=1-std::exp(-10*dt);
   cycle.lift+=(std::clamp((rear+front)*.035,0.0,2.5)-cycle.lift)*blend;
   cycle.pitch+=(std::clamp((rear-front)*.002,-.045,.045)-cycle.pitch)*blend;
   if(remaining-travel<=1e-8&&cycle.speed<=braking*dt){
    // Last subpixel displacement is real wheel travel too.
    double endpoint=t.direction>0?right:left;
    t.angle=std::remainder(t.angle+(endpoint-t.x)/10,2*pi);t.x=endpoint;
    cycle.speed=0;cycle.hauledArea+=t.cargo;t.cargo=0;
    t.direction=-t.direction;++cycle.passes;
    if(cycle.passes>=2||fullness<=.2){t.active=0;cycle.lift=0;cycle.pitch=0;cycle.heading=t.direction;continue;}
    cycle.turnRemaining=.45;continue;
   }
   double blade=t.x+t.direction*32;
   for(size_t i=0;i<s.h.size();++i){
    double width=std::min(bin,s.r.w-i*bin),cx=s.r.x+i*bin+width/2;
    if(std::abs(cx-blade)>12||s.h[i]<=0||clumps.size()>=maxClumps){continue;}
    double amount=s.h[i]*width*std::min(.6,dt*12);
    if(amount<.01){continue;}
    double loaded=std::min(amount*.5,std::max(0.0,42*d.w-t.cargo));
    scatter(cx,s.r.y-s.h[i],amount-loaded,t.direction*80,-100);
    t.cargo+=loaded;s.h[i]-=amount/width;
   }
  }
 }
 // Granular repose: one bounded pass; use actual widths for narrow end bins.
 for(auto &e:surfaces){
  auto &s=e.second;auto &h=s.h;
  // One local fracture front, hysteresis 12pt ignition / 2pt repose.
  // Scan at most 64 bonds and process at most 6 bonds per capped step.
  // Area moves between adjacent VISIBLE bins only; no whole-edge burst.
  if(s.avalanche.empty()&&h.size()>1){
   for(size_t scan=0;scan<std::min(size_t(64),h.size()-1);++scan){
    if(s.avalancheCursor>=h.size())s.avalancheCursor=1;
    size_t i=s.avalancheCursor++;
    if(s.visible[i]&&s.visible[i-1]&&std::abs(h[i]-h[i-1])>12){s.avalanche.push_back(i);break;}
   }
  }
  s.avalancheCredit=std::min(6.0,s.avalancheCredit+120*dt);
  auto enqueue=[&](size_t j){
   if(j>0&&j<h.size()&&s.avalanche.size()<32&&std::find(s.avalanche.begin(),s.avalanche.end(),j)==s.avalanche.end())s.avalanche.push_back(j);
  };
  while(!s.avalanche.empty()&&s.avalancheCredit>=1){
   size_t i=s.avalanche.front();s.avalanche.erase(s.avalanche.begin());s.avalancheCredit-=1;
   if(i==0||i>=h.size()||!s.visible[i]||!s.visible[i-1])continue;
   double diff=h[i]-h[i-1];if(std::abs(diff)<=2+1e-9)continue;
   double wa=std::min(bin,s.r.w-(i-1)*bin),wb=std::min(bin,s.r.w-i*bin);
   double transfer=(std::abs(diff)-2)/(1/wa+1/wb)*.15;
   if(diff>0){h[i]-=transfer/wb;h[i-1]+=transfer/wa;}
   else{h[i-1]-=transfer/wa;h[i]+=transfer/wb;}
   enqueue(i-1);enqueue(i+1);enqueue(i);
  }
  if(s.avalanche.empty())s.avalancheCredit=0;
  // Only exposed physical window ends, not occlusion boundaries or display floors.
  // Thin flat banks are stable; deep caps slough off unsupported outside faces.
  if(e.first<0||h.empty()){continue;}
  for(int side=0;side<2;++side){
   size_t i=side==0?0:h.size()-1,j=h.size()==1?i:(side==0?1:i-1);
   if(!s.visible[i]||h[i]<=8||clumps.size()>=maxClumps){continue;}
   double width=std::min(bin,s.r.w-i*bin);
   // A deep flat cap also has an unsupported outside face; it must not require
   // an artificially preloaded lip. Retain a 24-point stable edge bank.
   double excess=std::max(h[i]-24,std::min(h[i]-8,h[i]-h[j]-2));
   // Keep material supported until a bounded irregular fracture event. This
   // removes one-identical-grain-per-frame cadence without a dust multiplier.
   if(s.edgeWait[side]==0)s.edgeWait[side]=grainRange(.018,.058);
   s.edgeAge[side]+=dt;
   if(s.edgeAge[side]<s.edgeWait[side])continue;
   double elapsed=s.edgeAge[side];s.edgeAge[side]=0;s.edgeWait[side]=grainRange(.018,.058);
   double amount=std::max(0.0,excess)*(1-std::exp(-3*elapsed))*width*grainRange(.65,1.0);
   if(amount<.01){continue;}
   amount=std::min(amount,24.0);
   double r=std::sqrt(amount/pi),offset=grainRange(.01,1.4);
   double x=side==0?s.r.x-r-offset:s.r.x+s.r.w+r+offset;
   clumps.push_back(loose(x,s.r.y-r+grainRange(-.8,.8),amount,e.first,side==0?-15.0:15.0));
   h[i]-=amount/width;
  }
 }
 for(size_t i=0;i<balls.size();++i){auto &b=balls[i];if(b.r<=0||(int)i==held)continue;
  if(b.parent>=0&&b.parent<(int)balls.size()&&balls[b.parent].r>0){auto &p=balls[b.parent];b.x=p.x;b.y=p.y-p.r-b.r+2;continue;}
  b.parent=-1;double oldBottom=b.y+b.r;b.vy+=460*dt;b.x+=b.vx*dt;b.y+=b.vy*dt;b.angle+=b.vx*dt/std::max(1.0,b.r);b.vx*=std::exp(-2*dt);
  double floor=1e20;for(auto &e:surfaces){auto &s=e.second;if(b.x>=s.r.x&&b.x<s.r.x+s.r.w){size_t j=(size_t)((b.x-s.r.x)/bin);if(j<s.h.size()&&s.visible[j]){double y=contourTop(s,j,b.x);if(oldBottom<=y+3&&b.y+b.r>=y)floor=std::min(floor,y);}}}
  if(floor<1e19){
   // Only an actual impact, not the gravity increment of resting contact.
   if(b.vy>35){b.squash=std::min(.12,b.vy*.00035);b.squashV=0;b.tiltV=std::clamp(b.tiltV+b.vx*.002,-.6,.6);}
   b.y=floor-b.r;b.vy=0;
  }
  double maxY=-1e20;for(auto d:displays)maxY=std::max(maxY,d.y+d.h);if(b.y>maxY+500)b.r=0;
 }
 syncAssembly();
}
SnowDiagnostics snow_diagnostics(){
 SnowDiagnostics d{};d.grains=(int)clumps.size();d.bands=(int)shedding.size();d.shedLastStep=shedLastStep;
 for(const auto &e:surfaces){const auto &s=e.second;for(size_t i=0;i<s.h.size();++i)d.pileArea+=s.h[i]*std::min(bin,s.r.w-i*bin);}
 for(const auto &p:clumps)d.airborneArea+=p.area;
 for(const auto &p:shedding)d.pendingArea+=p.area;
 return d;
 monitorPrune(true);
}
int snow_sprites(SnowSprite *out,int cap){syncAssembly();int n=0;for(auto p:flakes)if(n<cap)out[n++]={p.x,p.y,p.r,.55+p.r*.13,0,0};
 for(auto p:clumps){if(n<cap){out[n++]={p.x,p.y,p.r,.90,5,p.angle};}}
 for(int i=0;i<(int)balls.size();++i) {auto b=balls[i];if(b.r>0&&n<cap)out[n++]={b.x,b.y,b.r,1,ballKind(i),b.angle}; }
 return n;}
int snow_piles(SnowPile *out,int cap){int n=0;for(auto &e:surfaces){auto &s=e.second;for(size_t i=0;i<s.h.size();++i)if(s.visible[i]&&s.h[i]>.05&&n<cap)out[n++]={s.r.x+i*bin,s.r.y-s.h[i],std::min(bin,s.r.w-i*bin),s.h[i]};}for(const auto &b:shedding){if(n<cap&&b.area/b.width>.05)out[n++]={b.x,b.bottom-b.area/b.width,b.width,b.area/b.width};}return n;}
void snow_down(double x,double y,int sweep){if(displays.empty()||monitorPoint(disabledDisplays,x,y))return;lastX=x;lastY=y;held=-1;if(sweep){sweepAir(x,y,x,y,0,-140);sweepAt(x,y);return;}
 for(int i=(int)balls.size()-1;i>=0;--i)if(balls[i].r>0&&std::hypot(balls[i].x-x,balls[i].y-y)<balls[i].r+8){held=i;balls[i].parent=-1;return;}
 double area=collect(x,y,20);if(area>=16&&balls.size()<128){balls.push_back({x,y,std::sqrt(area/pi)});held=(int)balls.size()-1;}}
void snow_drag_timed(double x,double y,int sweep,double dt){if(displays.empty()||monitorPoint(disabledDisplays,x,y)){held=-1;return;}
 // Invalid/zero event intervals cannot produce infinite launch velocities.
 double vx=std::clamp((x-lastX)/std::clamp(std::isfinite(dt)?dt:0.0,1.0/240,0.1),-500.0,500.0);
 double vy=std::clamp((y-lastY)/std::clamp(std::isfinite(dt)?dt:0.0,1.0/240,0.1),-350.0,150.0)-140;
 if(sweep){sweepAir(lastX,lastY,x,y,vx,vy);}
 double distance=std::hypot(x-lastX,y-lastY);int steps=std::max(1,(int)std::ceil(std::min(distance,5120.0)/5));
 for(int k=1;k<=steps;++k){double px=lastX+(x-lastX)*k/steps,py=lastY+(y-lastY)*k/steps;
  if(sweep){sweepAt(px,py,vx,vy);}else if(held>=0){auto &b=balls[held];b.tiltV=std::clamp(b.tiltV-(px-b.x)*.008,-.6,.6);b.flutterV=std::clamp(b.flutterV-(px-b.x)*.012,-.8,.8);double area=collect(px,py,b.r+8,std::max(0.0,pi*(48*48-b.r*b.r)));b.r=std::min(48.0,std::sqrt(b.r*b.r+area/pi));b.angle+=(px-b.x)/std::max(1.0,b.r);b.vx=std::clamp((px-b.x)*8,-250.0,250.0);b.x=px;b.y=py;}}
 lastX=x;lastY=y;syncAssembly();
}
void snow_drag(double x,double y,int sweep){snow_drag_timed(x,y,sweep,1.0/60);}
void snow_up(){if(held>=0){auto &b=balls[held];double nearest=1e20;int target=-1;
 for(int i=0;i<(int)balls.size();++i){auto &p=balls[i];if(i==held||p.r<b.r*1.08||p.r<=0)continue;
  // Decreasing radii prevent cycles. Maximum three levels.
  int depth=1,j=i;while(balls[j].parent>=0&&depth<4){j=balls[j].parent;++depth;}
  bool occupied=false;for(auto child:balls)if(child.r>0&&child.parent==i)occupied=true;
  double d=std::hypot(b.x-p.x,b.y-(p.y-p.r-b.r+2));if(depth<3&&!occupied&&d<38&&d<nearest){target=i;nearest=d;}}
 if(target>=0){int root=target;for(int depth=0;depth<3&&balls[root].parent>=0;++depth)root=balls[root].parent;balls[root].tiltV=std::clamp(balls[root].tiltV+.28,-.6,.6);b.parent=target;b.vx=b.vy=0;auto &p=balls[target];b.x=p.x;b.y=p.y-p.r-b.r+2;if(p.parent>=0)b.face=true;}
 }held=-1;syncAssembly();}
}
