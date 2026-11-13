#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
int main(){
 SnowRect ds[]={{-640,0,640,480},{0,0,640,480}};
 snow_displays(ds,2);snow_windows(nullptr,0);snow_reset();
 // Existing upward broom throws must still return, not be killed by policy.
 flakes.push_back({100,-80,2,40,0});snow_step(.01,0,0);
 assert(flakes.size()==1);
 std::cout<<"PASS monitor policy preserves existing offscreen/upward particle behavior\n";
 snow_corner_inset(0);
 SnowWindow w={90,{-200,200,400,100}};snow_windows(&w,1);
 for(auto &e:surfaces)for(auto &h:e.second.h)h=20;
 auto floor=surfaces[-2].h;
 snow_disabled_displays(ds,1);snow_displays(ds+1,1);snow_windows(&w,1);
 assert(surfaces[-1].h==floor);assert(surfaces.count(-2)==0);
 for(size_t i=0;i<surfaces[90].h.size();++i){auto &s=surfaces[90];double x=s.r.x+i*bin;if(x<0){assert(s.h[i]==0&&!s.visible[i]);}else{assert(s.h[i]==20&&s.visible[i]);}}
 std::cout<<"PASS negative-coordinate spanning support clipped; surviving floor and window snow preserved\n";
 assert(monitorBlockedShelf(disabledDisplays,-4,200,8));assert(!monitorBlockedShelf(disabledDisplays,0,200,8));
 std::vector<SnowRect> upper={{0,-480,640,480}};assert(monitorBlockedShelf(upper,0,-1,8));
 std::cout<<"PASS partial seam bins and vertically excluded supports cannot collide\n";
 // Disabled monitor is a real exclusion region; enabled material survives.
 snow_disabled_displays(ds,1);
 flakes={{100,100,2,40,0},{-100,100,2,40,0},{100,-80,2,40,0}};
 clumps={{100,100,2,0,12,90},{-100,100,2,0,12,90}};
 balls={{-100,100,20},{100,100,12,0,0,0,0}};
 held=0;monitorPrune();
 assert(flakes.size()==2&&clumps.size()==1&&balls[0].r==0&&balls[1].r==12&&balls[1].parent==-1&&held==-1);
 snow_down(-100,100,0);assert(held==-1);
 // A captured drag may enter a disabled display: it must cancel, not orphan.
 held=1;snow_drag_timed(-100,100,0,.02);assert(held==-1);
 std::cout<<"PASS disabled monitor drops only its airborne/balls and cancels captured interaction; enabled throw preserved\n";
 snow_displays(nullptr,0);snow_windows(&w,1);
 assert(tractors.empty()&&cycles.empty()&&flakes.empty()&&clumps.empty()&&shedding.empty());
 for(auto &e:surfaces)for(auto h:e.second.h)assert(h==0);
 auto before=simulationTime;snow_step(.05,3,0);assert(simulationTime==before);
 snow_disabled_displays(nullptr,0);snow_displays(ds,2);snow_windows(nullptr,0);snow_step(.05,3,0);assert(!flakes.empty());
 std::cout<<"PASS all-disabled physics idle and re-enable works\n";
 snow_displays(ds,2);snow_windows(nullptr,0);snow_reset();
 for(auto &h:surfaces[-1].h){h=12;}
 for(auto &h:surfaces[-2].h){h=27;}
 SnowRect reversed[]={ds[1],ds[0]};snow_displays(reversed,2);snow_windows(nullptr,0);
 assert(surfaces[-1].h[0]==27&&surfaces[-2].h[0]==12);
 std::cout<<"PASS hotplug/enumeration reordering preserves floor by display geometry, not vector index\n";
}
