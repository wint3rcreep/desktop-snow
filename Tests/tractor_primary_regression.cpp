// Minimal regression also compiles against unmodified v2.3 (no cycle internals).
#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
int main(){
 snow_corner_inset(0); // Legacy fixtures explicitly assume square windows.
 SnowRect d={0,0,640,480};snow_displays(&d,1);snow_windows(nullptr,0);snow_reset();snow_tractor_enabled(1);
 auto &h=surfaces[-1].h;std::fill(h.begin(),h.end(),35);snow_step(.01,2.5,0);
 assert(tractors[0].active);double dir=tractors[0].direction;int turns=0;
 for(int k=0;k<3000;++k){std::fill(h.begin(),h.end(),21);snow_step(.01,2.5,0);if(dir!=tractors[0].direction){++turns;dir=tractors[0].direction;}}
 std::cout<<"started above80%, sustained50% residual / blizzard: turns="<<turns<<" active="<<tractors[0].active<<std::endl;
 assert(turns<=2&&!tractors[0].active);
 std::cout<<"PASS primary tractor regression independent of new cycle internals\n";
}
