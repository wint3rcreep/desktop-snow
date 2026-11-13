#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
double mass(){double a=0;for(const auto &e:surfaces)for(size_t i=0;i<e.second.h.size();++i)a+=e.second.h[i]*std::min(bin,e.second.r.w-i*bin);for(auto p:clumps)a+=p.area;for(auto p:shedding)a+=p.area;return a;}
int main(){
 snow_corner_inset(0); // Legacy fixtures explicitly assume square windows.
 SnowRect d={0,0,1200,1000};snow_displays(&d,1);
 SnowWindow w={10,{300,200,320,80}};snow_windows(&w,1);snow_reset();
 std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);double initial=mass();
 snow_step(.03,0,0);w.rect.x+=8;snow_windows(&w,1);
 assert(!shedding.empty()&&clumps.empty());assert(std::abs(mass()-initial)<1e-8);
 const size_t bands=shedding.size();snow_windows(&w,1);assert(shedding.size()==bands);
 w.rect.x+=.2;snow_windows(&w,1);assert(shedding.size()==bands);
 size_t activeFrames=0,previous=0;double maxGrain=0;
 // v3.0 slow shedding can last .70s plus seeded start delay.
 for(int k=0;k<90;++k){snow_step(.01,0,0);if(clumps.size()>previous)++activeFrames;previous=clumps.size();
  assert(clumps.size()<=maxClumps);assert(std::abs(mass()-initial)<1e-7);
  for(auto p:clumps){assert(p.source==10&&std::abs(p.vx)<=120);assert(p.x<665);maxGrain=std::max(maxGrain,p.area);}
 }
 assert(activeFrames>=8&&shedding.empty()&&maxGrain<=32);
 std::cout<<"PASS staged shedding frames="<<activeFrames<<" max_grain_area="<<maxGrain<<" conserved="<<mass()<<"\n";
 for(int i=0;i<500;++i)snow_step(.01,0,0);
 std::cerr<<"DIAG clumps="<<clumps.size()<<" pending="<<shedding.size()<<" mass="<<mass()<<" initial="<<initial<<"\n";
 assert(clumps.empty()&&std::abs(mass()-initial)<1e-6);
 std::cout<<"PASS released snow settles without duplicate mass or source recapture\n";
 snow_reset();std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 w.rect.x+=10000;snow_windows(&w,1);for(auto b:shedding)assert(b.vx==0);
 snow_step(.02,0,0);snow_reset();assert(shedding.empty()&&clumps.empty()&&mass()==0);
 std::cout<<"PASS teleports suppress inherited velocity; Clear removes pending release\n";
 snow_windows(&w,1);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);snow_windows(nullptr,0);
 assert(!shedding.empty());snow_forget_windows();assert(shedding.empty());
 std::cout<<"PASS removal queues once; Space reset discards pending world snow\n";
 snow_windows(nullptr,0);clumps={{400,100,2,10,12,10,100}};
 snow_step(.02,0,0);double x=clumps[0].x,y=clumps[0].y;
 snow_reset();clumps={{400,100,2,10,12,10,100}};snow_step(.01,0,0);snow_step(.01,0,0);
 assert(std::abs(clumps[0].x-x)<1e-10&&std::abs(clumps[0].y-y)<1e-10);
 snow_step(std::nan(""),0,0);assert(std::isfinite(clumps[0].x));
 std::cout<<"PASS damped horizontal and gravitational vertical integration subdivide; NaN dt inert\n";
 snow_reset();w={10,{300,200,320,80}};snow_windows(&w,1);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 w.rect.x+=2;snow_windows(&w,1);initial=mass();clumps.assign(maxClumps,{400,100,1,0,1,10});
 initial+=maxClumps;snow_step(.02,0,0);assert(std::abs(mass()-initial)<1e-7&&shedding.size()<=maxShedding);
 std::cout<<"PASS saturated airborne budget backpressures pending shedding without mass deletion\n";
 auto diag=snow_diagnostics();assert(diag.grains==(int)clumps.size()&&diag.bands==(int)shedding.size());
 assert(std::abs(diag.pileArea+diag.pendingArea+diag.airborneArea-mass())<1e-7);
 std::cout<<"PASS public diagnostics count all accepted area including detached reservoirs\n";
 snow_reset();snow_windows(&w,1);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 double oldSupportX=w.rect.x;w.rect.x+=20;snow_windows(&w,1);
 SnowPile exportPiles[4096];int count=snow_piles(exportPiles,4096);assert(count==40);
 for(int j=0;j<count;++j){assert(exportPiles[j].x>=oldSupportX&&exportPiles[j].x<oldSupportX+320);assert(exportPiles[j].y+exportPiles[j].h==200);}
 std::cout<<"PASS exported transitional contours stay at old support, not translated with window\n";
 snow_reset();snow_windows(&w,1); // v3.0 Clear also resets timestamp/velocity history.
 surfaces[10].vx=0;simulationTime+=.03;std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);
 w.rect.x+=100;snow_windows(&w,1);double firstVelocity=shedding[0].vx;assert(firstVelocity>0&&firstVelocity<=120);
 snow_step(.03,0,0);std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);w.rect.x+=100;snow_windows(&w,1);
 assert(shedding.back().vx>=firstVelocity&&shedding.back().vx<=120);
 std::cout<<"PASS support velocity inherited gradually and clamped even for fast drag\n";
 snow_reset();std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),42);w.rect.x+=2;snow_windows(&w,1);
 snow_step(100,0,0);auto capped=snow_diagnostics();assert(capped.shedLastStep<=96);
 snow_reset();std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),42);w.rect.x+=2;snow_windows(&w,1);
 snow_step(.05,0,0);auto bounded=snow_diagnostics();assert(capped.shedLastStep==bounded.shedLastStep&&std::abs(capped.pendingArea-bounded.pendingArea)<1e-8);
 std::cout<<"PASS release production bounded to96 per step and sleep dt equals50ms\n";
 snow_reset();double expected=0;
 for(int k=0;k<110;++k){std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);expected+=320*12;w.rect.x+=2;snow_windows(&w,1);}
 assert(shedding.size()==maxShedding&&std::abs(mass()-expected)<1e-6);
 std::cout<<"PASS4096 reservoir cap coalesces overload conservatively instead of losing released snow\n";

}
