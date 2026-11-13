// Include the production translation unit to exercise internal fixtures too.
#include "../Core/Snow.cpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
// Legacy pile fixtures measure live supports only. v2.5 also exports old-world
// detached reservoir contours for a short visual transition; tested separately.
std::vector<SnowPile> piles(){std::vector<SnowPile> p;for(const auto &e:surfaces){const auto &s=e.second;for(size_t i=0;i<s.h.size();++i)if(s.visible[i]&&s.h[i]>.05)p.push_back({s.r.x+i*bin,s.r.y-s.h[i],std::min(bin,s.r.w-i*bin),s.h[i]});}return p;}
// Conservation includes sub-render-threshold material, not only visible pile exports.
double area(){return snow_diagnostics().pileArea;}
void fill(){for(int i=0;i<1800;++i)snow_step(1.0/60,3,0);}
int main(){
 snow_corner_inset(0); // Legacy fixtures explicitly assume square windows.
 SnowRect d={0,0,640,480};snow_displays(&d,1);
 SnowWindow w={123,{100,200,300,180}};snow_windows(&w,1);snow_reset();fill();
 auto p=piles();bool onWindow=false,onFloor=false;for(auto s:p){onWindow|=std::abs(s.y+s.h-200)<.01;onFloor|=std::abs(s.y+s.h-479)<.01;assert(s.h<=42.001);}
 assert(onWindow&&onFloor);std::cout<<"PASS falling particles land on real surfaces and floor; pile height bounded\n";
 double before=area();snow_step(0,3,100);assert(area()==before);snow_step(-1,3,0);assert(area()==before);std::cout<<"PASS zero and negative dt inert\n";
 w.rect.x+=50;w.rect.y+=20;snow_windows(&w,1);
 assert(!shedding.empty());for(auto q:piles()){assert(std::abs(q.y+q.h-220)>.01);}
 std::cout<<"PASS moved window releases pile at old coordinates instead of carrying it\n";
 fill();
 SnowWindow list[2]={{456,{200,100,100,250}},w};snow_windows(list,2);for(auto s:piles())if(std::abs(s.y+s.h-220)<.01)assert(s.x<200||s.x>=300);std::cout<<"PASS front window masks occluded top-edge bins\n";
 snow_windows(&w,1);before=area();snow_down(175,220,0);snow_drag(240,220,0);snow_up();SnowSprite sprites[10000];int n=snow_sprites(sprites,10000);bool ball=false;for(int i=0;i<n;++i)ball|=sprites[i].kind==1;assert(ball&&area()<before);std::cout<<"PASS rolling consumes real pile area and creates growing ball\n";
 before=area();snow_down(400,479,1);snow_drag(0,479,1);snow_up();assert(area()<before);std::cout<<"PASS continuous broom scatters piles instead of deleting\n";
 snow_windows(nullptr,0);for(auto s:piles())assert(std::abs(s.y+s.h-479)<.01);std::cout<<"PASS closed windows removed\n";
 snow_reset();snow_step(100,1,0);int capped=snow_sprites(sprites,10000);snow_reset();snow_step(.05,1,0);assert(snow_sprites(sprites,10000)==capped);std::cout<<"PASS frame delta capped after sleep\n";
 snow_reset();assert(piles().empty());assert(snow_sprites(sprites,10000)==0);std::cout<<"PASS clear resets piles particles and balls\n";
 // Deterministic fixtures, then the public mouse API does all stacking.
 balls={{300,449,30},{200,400,20},{100,350,12}};
 snow_down(200,400,0);snow_drag(300,401,0);snow_up();
 assert(balls[1].parent==0);
 snow_down(100,350,0);snow_drag(300,371,0);snow_up();
 assert(balls[2].parent==1&&balls[2].face);
 snow_step(.016,0,0);assert(std::abs(balls[2].x-balls[0].x)<.001);
 std::cout<<"PASS three distinct balls stack into snowman with face\n";
 snow_down(300,449,0);snow_drag(350,449,0);snow_up();snow_step(.016,0,0);
 assert(std::abs(balls[2].x-balls[0].x)<.001);
 std::cout<<"PASS stacked children follow dragged base\n";
 snow_down(balls[0].x,balls[0].y,1);snow_up();snow_step(.016,0,0);
 assert(balls[1].parent==-1);std::cout<<"PASS sweeping base safely releases stacked children\n";
 SnowRect two[2]={{-640,0,640,480},{0,-480,640,480}};
 snow_reset();snow_displays(two,2);snow_windows(nullptr,0);fill();
 bool left=false,upper=false;for(auto s:piles()){left|=s.x<0;upper|=s.y+s.h<1;}
 assert(left&&upper);std::cout<<"PASS negative-coordinate and vertically arranged displays\n";
 SnowWindow equalTop[2]={{99,{150,220,120,160}},w};
 snow_windows(equalTop,2);
 for(size_t i=0;i<surfaces[123].visible.size();++i){double center=150+(i+.5)*8;if(center<270)assert(!surfaces[123].visible[i]);}
 std::cout<<"PASS aligned front window hides coincident back top edge\n";
 // v2 regression fixtures use the production graph and public mouse/export API.
 snow_reset();balls={{300,449,30},{200,400,20},{100,350,12}};
 auto kinds=[&](){SnowSprite out[16];int count=snow_sprites(out,16);std::vector<int> k;for(int i=0;i<count;++i)k.push_back(out[i].kind);return k;};
 assert((kinds()==std::vector<int>{1,1,1}));
 snow_down(200,400,0);snow_drag(300,401,0);snow_up();
 assert((kinds()==std::vector<int>{1,1,1}));
 snow_down(100,350,0);snow_drag(300,371,0);snow_up();
 assert((kinds()==std::vector<int>{4,3,2}));
 std::cout<<"PASS accessories only on complete three-ball assembly; head/torso/base roles exported\n";
 snow_down(300,449,0);snow_drag(380,420,0);
 assert(balls[1].x==380&&balls[2].x==380);
 assert(balls[1].y==balls[0].y-balls[0].r-balls[1].r+2);
 assert(balls[2].y==balls[1].y-balls[1].r-balls[2].r+2);
 assert((kinds()==std::vector<int>{4,3,2}));snow_up();
 std::cout<<"PASS assembly and accessories follow base immediately without physics tick (paused too)\n";
 snow_down(balls[2].x,balls[2].y,0);
 assert((kinds()==std::vector<int>{1,1,1}));snow_drag(600,100,0);snow_up();
 assert((kinds()==std::vector<int>{1,1,1}));
 std::cout<<"PASS detaching head removes all accessories and stale face\n";
 // Reverse creation order: head precedes its ancestors in storage.
 snow_reset();balls={{300,371,12},{300,401,20},{300,449,30}};
 balls[0].parent=1;balls[1].parent=2;
 assert((kinds()==std::vector<int>{2,3,4}));
 snow_down(300,449,0);snow_drag(390,430,0);
 assert(balls[0].x==390&&balls[0].y==balls[1].y-balls[1].r-balls[0].r+2);
 snow_up();snow_step(.016,0,0);
 assert(balls[0].y==balls[1].y-balls[1].r-balls[0].r+2);
 std::cout<<"PASS parent-first following independent of creation order\n";
 balls[2].r=0;assert((kinds()==std::vector<int>{1,1}));
 snow_step(.016,0,0);assert((kinds()==std::vector<int>{1,1}));
 std::cout<<"PASS removed base cannot retain complete-assembly accessories\n";
 assert(snow_window_interval(0,1,0)==1.0/30);
 assert(snow_window_interval(0,0,1)==1.0/30);
 assert(snow_window_interval(0,0,0)==.12);
 assert(snow_window_interval(1,1,1)==.5);
 std::cout<<"PASS production tracking policy: 30Hz drag/motion, idle 120ms, low-power 500ms\n";
 snow_reset();snow_displays(&d,1);snow_windows(&w,1);
 auto &surf=surfaces[w.id];std::fill(surf.h.begin(),surf.h.end(),10);
 double initial=area();snow_windows(&w,1);assert(clumps.empty()&&shedding.empty()&&area()==initial);
 w.rect.x+=.2;snow_windows(&w,1);assert(clumps.empty());
 w.rect.x+=1;snow_windows(&w,1);assert(!shedding.empty()&&piles().empty());
 for(int frame=0;frame<15&&clumps.empty();++frame)snow_step(.01,0,0);
 assert(!clumps.empty());
 size_t released=clumps.size();double oldX=clumps[0].x,oldY=clumps[0].y,oldVX=clumps[0].vx,oldVY=clumps[0].vy;
 snow_windows(&w,1);assert(clumps.size()==released);
 snow_step(.02,0,0);assert(std::abs(clumps[0].x-oldX-oldVX*(1-std::exp(-1.5*.02))/1.5)<1e-9);
 assert(std::abs(clumps[0].y-oldY-oldVY*.02-230*.02*.02)<1e-9);
 std::cout<<"PASS unchanged/jitter polls preserve snow; move emits once at old position with gravity\n";
 snow_forget_windows();snow_windows(&w,1);assert(clumps.empty());
 SnowWindow stacked[2]={{10,{100,100,160,80}},{20,{80,300,240,100}}};
 snow_reset();snow_windows(stacked,2);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 initial=area();snow_windows(stacked+1,1);assert(!shedding.empty());
 assert(area()==0);for(int i=0;i<120;++i){snow_step(.01,0,0);}
 std::cerr<<"DIAG clumps="<<clumps.size()<<" pending="<<shedding.size()<<" mass="<<area()<<" initial="<<initial<<"\n";
 assert(clumps.empty()&&std::abs(area()-initial)<.001);
 for(auto q:piles()){assert(std::abs(q.y+q.h-300)<.001);}
 std::cout<<"PASS close/minimize releases pile; lower visible window captures conserved area\n";
 snow_windows(nullptr,0);assert(!shedding.empty());
 for(int i=0;i<150;++i){snow_step(.01,0,0);}
 assert(clumps.empty()&&std::abs(area()-initial)<.001);
 std::cout<<"PASS lower support removal falls onto floor conserving area\n";
 snow_forget_windows();snow_windows(stacked,2);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 stacked[0].rect.x+=20;stacked[0].rect.y+=20;snow_windows(stacked,2);
 assert(!shedding.empty());for(int i=0;i<20;++i){snow_step(.01,0,0);}
 assert(surfaces[10].h[0]==0&& !clumps.empty());
 std::cout<<"PASS overlapping downward move does not immediately recapture released snow\n";
 for(int i=0;i<150;++i){snow_step(.01,0,0);}
 assert(clumps.empty());double lowerArea=0;
 for(auto q:piles()){if(std::abs(q.y+q.h-300)<.001){lowerArea+=q.w*q.h;}}
 assert(lowerArea>0);
 std::cout<<"PASS moved-window snow settles on lower visible support\n";
 // Fresh flakes can land on the moving source, but its old clumps cannot.
 flakes.push_back({stacked[0].rect.x+12,stacked[0].rect.y-1,1,80,0});
 snow_step(.02,0,0);assert(surfaces[10].h[1]>0);
 stacked[0].rect.x+=3;snow_windows(stacked,2);
 for(int frame=0;frame<30&&clumps.empty();++frame)snow_step(.01,0,0);
 assert(!clumps.empty());
 double startY=clumps[0].y,startVy=clumps[0].vy;
 snow_step(.01,0,0);snow_step(.01,0,0);double splitY=clumps[0].y;
 clumps[0].y=startY;clumps[0].vy=startVy;snow_step(.02,0,0);
 assert(std::abs(splitY-clumps[0].y)<1e-9);
 std::cout<<"PASS fresh flakes accumulate during motion; gravity invariant under dt subdivision\n";
 snow_forget_windows();assert(clumps.empty()&&piles().empty());
 snow_windows(stacked,2);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 stacked[0].rect.w-=16;snow_windows(stacked,2);assert(shedding.size()==2&&surfaces[10].h[0]==12);
 stacked[0].rect.h+=10;released=clumps.size();snow_windows(stacked,2);assert(clumps.size()==released);
 std::cout<<"PASS width shrink releases removed edge; height-only resize preserves pile\n";
 for(int k=0;k<300;++k){
  std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),42);
  stacked[0].rect.x+=2;snow_windows(stacked,2);
  assert(clumps.size()<=maxClumps);
 }
 assert(shedding.size()<=maxShedding);
 for(int i=0;i<50;++i)snow_step(.01,0,0);
 double airborne=0;for(auto q:clumps){airborne+=q.area;assert(std::isfinite(q.y));}
 assert(airborne<=maxClumps*32);
 snow_reset();assert(clumps.empty());
 std::cout<<"PASS repeated moving releases bounded 2048 grains and 4096 pending bands; reset clears them\n";
 // Broom regressions: use production mouse handlers and airborne state.
 snow_forget_windows();snow_displays(&d,1);snow_windows(nullptr,0);snow_reset();
 auto seedFloor=[](){auto &s=surfaces[-1];std::fill(s.h.begin(),s.h.end(),0);s.h[40]=10;};
 seedFloor();initial=area();snow_down(260,479,1);
 snow_drag_timed(330,479,1,.05);snow_up();
 assert(!clumps.empty()&&area()==0&&clumps[0].vx>0);
 oldX=clumps[0].x;oldY=clumps[0].y;snow_step(.02,0,0);
 assert(clumps[0].x>oldX&&clumps[0].y<oldY);
 for(int i=0;i<300;++i){snow_step(.01,0,0);}
 assert(clumps.empty()&&std::abs(area()-initial)<.001);
 std::cout<<"PASS broom emits mobile upward/right snow and redeposits conserved area on same floor\n";
 snow_reset();seedFloor();snow_down(390,479,1);snow_drag_timed(320,479,1,0);snow_up();
 assert(!clumps.empty()&&clumps[0].vx<0&&std::isfinite(clumps[0].vx)&&std::abs(clumps[0].vx)<=500);
 std::cout<<"PASS broom drag direction reverses impulse; zero event dt is finite and clamped\n";
 snow_reset();balls={{300,449,30},{300,401,20},{300,371,12}};
 balls[1].parent=0;balls[2].parent=1;
 snow_down(300,449,1);snow_drag_timed(310,449,1,.01);snow_up();
 assert(!clumps.empty());for(auto &b:balls){if(b.r>0){assert(b.parent<0||balls[b.parent].r>0);}}
 for(int i=0;i<300;++i){snow_step(.01,0,0);}
 assert(area()>0);for(auto &b:balls){assert(!b.face);}
 std::cout<<"PASS broom breaks snowman into falling snow; surviving children detach safely\n";
 snow_reset();
 for(int i=0;i<3000;++i){seedFloor();snow_down(324,479,1);snow_up();}
 assert(clumps.size()==maxClumps);
 for(auto q:clumps){assert(std::isfinite(q.x)&&std::isfinite(q.vx));}
 snow_reset();assert(clumps.empty()&&piles().empty());
 std::cout<<"PASS broom shares bounded airborne budget; Clear still deletes snow\n";

 // v2.3: airborne snowfall must respond through the real mouse API.
 snow_reset();snow_windows(nullptr,0);flakes={{320,200,2,50,0}};
 snow_down(260,200,1);snow_drag_timed(320,200,1,.05);snow_up();
 oldX=flakes[0].x; snow_step(.02,0,0);
 assert(flakes.size()==1&&flakes[0].x>oldX+1);
 std::cout<<"PASS airborne flake deflects right through production mouse dispatch\n";
 for(auto delta:std::vector<std::pair<double,double>>{{-60,0},{0,-60},{0,60}}){
  snow_reset();flakes={{320,240,2,50,0}};
  snow_down(320-delta.first,240-delta.second,1);
  snow_drag_timed(320,240,1,.05);snow_up();
  oldX=flakes[0].x;oldY=flakes[0].y;snow_step(.02,0,0);
  assert(flakes.size()==1);
  if(delta.first<0){assert(flakes[0].x<oldX-1);}
  if(delta.second<0){assert(flakes[0].y<oldY-1);}
  if(delta.second>0){assert(flakes[0].y>oldY+1);}
 }
 std::cout<<"PASS airborne flakes deflect left upward and downward\n";
 snow_reset();flakes={{320,240,2,50,0},{550,240,2,50,0}};
 clumps={{320,240,3,30,pi*9,123}};
 snow_down(260,240,1);snow_drag_timed(320,240,1,.05);snow_up();
 assert(clumps.size()==1&&clumps[0].vx>0&&clumps[0].source==123);
 assert(flakes[1].vx==0&&flakes[1].vy==0);
 assert(flakes[0].x==320&&flakes[0].y==240&&clumps[0].x==320);
 snow_step(.02,0,0);assert(clumps[0].x>320);
 std::cout<<"PASS existing airborne clumps deflect without respawn; distant flakes unaffected; no teleport\n";
 snow_reset();flakes={{320,240,2,50,0}};
 snow_down(320,240,0);snow_drag_timed(321,240,0,0);snow_up();
 assert(flakes[0].vx==0&&flakes[0].vy==0);
 snow_down(320,240,1);snow_drag_timed(320,240,1,0);snow_up();
 snow_step(0,0,0);assert(flakes[0].x==320&&flakes[0].y==240);
 for(int i=0;i<10000;++i){snow_down(320,240,1);snow_drag_timed(320,240,1,0);snow_up();}
 assert(flakes.size()==1&&clumps.empty());
 assert(std::isfinite(flakes[0].vx)&&std::isfinite(flakes[0].vy));
 assert(std::abs(flakes[0].vx)<=500&&std::abs(flakes[0].vy)<=490);
 for(int i=0;i<1200;++i){snow_step(.01,0,0);}
 assert(flakes.empty()&&area()>0);
 std::cout<<"PASS non-broom leaves air unchanged; zero distance/dt finite; repeated impulses bounded; flakes settle\n";
 snow_reset();SnowWindow ledge={900,{100,200,83,100}};snow_windows(&ledge,1);
 surfaces[900].h.back()=20;initial=area();
 // v2.8 fractures are event-timed, not required on the first animation frame.
 for(int frame=0;frame<8&&clumps.empty();++frame)snow_step(.01,0,0);
 assert(!clumps.empty()&&area()<initial);
 std::cout<<"PASS loose snow spills gradually from support edge\n";
 auto totalMass=[](){double a=0;for(const auto &e:surfaces){for(size_t i=0;i<e.second.h.size();++i){a+=e.second.h[i]*std::min(bin,e.second.r.w-i*bin);}}for(auto q:clumps){a+=q.area;}return a;};
 assert(std::abs(totalMass()-initial)<1e-9);
 for(int i=0;i<400;++i){snow_step(.01,0,0);}
 assert(clumps.empty()&&std::abs(totalMass()-initial)<1e-8&&std::any_of(surfaces[-1].h.begin(),surfaces[-1].h.end(),[](double h){return h>0;}));
 std::cout<<"PASS partial-width edge spill conserves area and settles on lower floor\n";
 snow_reset();snow_windows(nullptr,0);auto &floorPile=surfaces[-1].h;
 floorPile[40]=30;initial=totalMass();
 // v3.0 bounded local front reaches the opposite neighbour on later ticks.
 for(int frame=0;frame<4;++frame)snow_step(.02,0,0);
 assert(floorPile[40]<30&&floorPile[39]>0&&floorPile[41]>0&&std::abs(totalMass()-initial)<1e-9);
 snow_reset();std::fill(floorPile.begin(),floorPile.end(),10);floorPile[40]=11;
 auto stable=floorPile;snow_step(.05,0,0);assert(floorPile==stable);
 snow_reset();floorPile[40]=30;snow_step(100,0,0);auto cappedPile=floorPile;
 snow_reset();floorPile[40]=30;snow_step(.05,0,0);assert(floorPile==cappedPile);
 std::cout<<"PASS granular slopes redistribute conservatively; sub-repose piles stable; dt bounded\n";
 snow_reset();snow_windows(&ledge,1);surfaces[900].h.back()=30;
 clumps.assign(maxClumps,{300,100,1,0,pi,900});initial=totalMass();
 snow_step(.01,0,0);assert(clumps.size()==maxClumps&&std::abs(totalMass()-initial)<1e-8);
 std::cout<<"PASS full airborne budget retains edge snow instead of deleting it\n";
 snow_reset();snow_forget_windows();snow_displays(two,2);snow_windows(nullptr,0);
 auto fillGround=[](int display,double h){auto &s=surfaces[-1-display];std::fill(s.h.begin(),s.h.end(),h);};
 fillGround(0,42);fillGround(1,42);snow_tractor_enabled(0);
 double parkedX=tractors[0].x;initial=totalMass();snow_step(.02,0,0);
 assert(tractors[0].x==parkedX&&clumps.empty()&&totalMass()==initial);
 SnowTractor tout[4];assert(snow_tractors(tout,4)==0);
 snow_reset();fillGround(0,42*.8);fillGround(1,0);snow_tractor_enabled(1);snow_step(.02,0,0);
 assert(!tractors[0].active&&!tractors[1].active&&snow_tractors(tout,4)==0);
 fillGround(0,42*.81);initial=totalMass();snow_step(.02,0,0);
 assert(tractors[0].active&&!tractors[1].active&&tractors[0].x>parkedX&&snow_tractors(tout,4)==1);
 assert(!clumps.empty()&&tractors[0].cargo>0);
 assert(std::abs(totalMass()+tractors[0].cargo-initial)<1e-8);
 std::cout<<"PASS tractor disabled inert; exact 80 percent parked; greater than 80 starts per-display plowing\n";
 fillGround(0,42*.5);snow_step(.02,0,0);assert(tractors[0].active);
 tractors[0].x=two[0].x+two[0].w-40-.1;tractors[0].direction=1;
 // v3.2 braking is distance/acceleration bounded, not an instantaneous 70pt/s hop.
 // A teleported fixture can require several steps to finish its short brake segment.
 for(int i=0;i<100&&tractors[0].direction==1;++i)snow_step(.02,0,0);
 assert(tractors[0].direction==-1&&tractors[0].x<=two[0].x+two[0].w-40);
 fillGround(0,42*.1);tractors[0].x=two[0].x+40+.1;tractors[0].direction=-1;
 snow_step(.02,0,0);assert(!tractors[0].active&&tractors[0].cargo==0);
 std::cout<<"PASS tractor hysteresis below 80; reverses at edge; unloads and parks below 20 percent\n";
 snow_reset();SnowWindow high={990,{-500,200,160,80}};snow_windows(&high,1);
 std::fill(surfaces[990].h.begin(),surfaces[990].h.end(),10);
 auto windowPile=surfaces[990].h;fillGround(0,40);fillGround(1,40);
 for(int k=0;k<1000;++k){
  snow_step(.05,0,0);
  for(size_t i=0;i<tractors.size();++i){
   assert(tractors[i].x>=two[i].x+40&&tractors[i].x<=two[i].x+two[i].w-40);
   assert(tractors[i].y==two[i].y+two[i].h-1&&std::isfinite(tractors[i].angle));
   assert(tractors[i].cargo<=42*two[i].w&&clumps.size()<=maxClumps);
  }
 }
 assert(surfaces[990].h==windowPile);
 std::cout<<"PASS tractors stay on own negative/upper display; windows untouched; cargo and clumps bounded\n";
 snow_reset();fillGround(0,40);fillGround(1,0);
 clumps.assign(maxClumps,{0,-400,1,0,pi,990});initial=totalMass();
 snow_step(.001,0,0);assert(clumps.size()==maxClumps&&tractors[0].cargo==0&&std::abs(totalMass()-initial)<1e-8);
 snow_tractor_enabled(0);assert(snow_tractors(tout,4)==0);
 std::cout<<"PASS saturated airborne budget keeps plow snow on ground; menu disable hides tractors\n";
 snow_reset();snow_displays(&d,1);snow_windows(nullptr,0);
 for(auto delta:std::vector<std::pair<double,double>>{{60,0},{-60,0},{0,-60},{0,60}}){
  snow_reset();clumps={{320,240,3,30,pi*9,123}};
  snow_down(320-delta.first,240-delta.second,1);snow_drag_timed(320,240,1,.05);snow_up();
  snow_step(.02,0,0);assert(clumps.size()==1);
  if(delta.first>0){assert(clumps[0].x>320);}
  if(delta.first<0){assert(clumps[0].x<320);}
  if(delta.second<0){assert(clumps[0].y<240);}
  if(delta.second>0){assert(clumps[0].y>240);}
  for(int i=0;i<500;++i){snow_step(.01,0,0);}
  assert(clumps.empty()&&std::abs(totalMass()-pi*9)<1e-8);
 }
 std::cout<<"PASS airborne clumps deflect in all four directions and settle with conserved area\n";
 snow_reset();flakes={{320,200,2,50,0},{320,244,2,50,0},{320,246,2,50,0}};
 snow_down(260,200,1);snow_drag_timed(320,200,1,.05);snow_up();
 assert(flakes[0].vx>flakes[1].vx&&flakes[1].vx>0&&flakes[2].vx==0);
 snow_reset();flakes={{320,1,2,24,0}};
 snow_down(320,61,1);snow_drag_timed(320,1,1,.05);snow_up();
 for(int i=0;i<200;++i){snow_step(.01,0,0);assert(flakes.size()==1);}
 for(int i=0;i<3000;++i){snow_step(.01,0,0);}
 assert(flakes.empty()&&totalMass()>0);
 std::cout<<"PASS smooth local falloff; upward throws near display top survive and return to settle\n";
 snow_reset();snow_displays(&d,1);snow_windows(nullptr,0);snow_tractor_enabled(1);fillGround(0,40);
 for(int i=0;i<20000;++i){snow_step(.05,0,0);}
 assert(!tractors[0].active&&tractors[0].cargo==0&&clumps.empty());
 assert(totalMass()<=.2*42*d.w);
 snow_tractor_enabled(0);
 std::cout<<"PASS automatic tractor actually completes cleanup and parks without new snowfall\n";
}
