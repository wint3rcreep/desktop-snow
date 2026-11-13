#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
void ground(int k,double h){auto &s=surfaces[-1-k];std::fill(s.h.begin(),s.h.end(),h);}
int main(){
 snow_corner_inset(0); // Legacy fixtures explicitly assume square windows.
 SnowRect ds[2]={{0,0,640,480},{640,0,320,480}};snow_displays(ds,2);snow_windows(nullptr,0);snow_reset();snow_tractor_enabled(1);
 ground(0,35);snow_step(.01,2.5,0);assert(tractors[0].active&&!tractors[1].active);
 int reversals=0;double dir=tractors[0].direction;
 // Original reported failure: >20% after a pass, never80 again. Keep50% floor
 // (blizzard replenishment controlled to isolate scheduling, not fake cleaning).
 for(int k=0;k<3000;++k){ground(0,21);snow_step(.01,2.5,0);if(tractors[0].direction!=dir){++reversals;dir=tractors[0].direction;}}
 std::cout<<"50% residual: reversals="<<reversals<<" active="<<tractors[0].active<<std::endl;
 assert(reversals==2&&!tractors[0].active&&cycles[0].passes==2);
 std::cout<<"PASS unfinished >20% cycle hard-stops after two edge-to-edge passes; no80 retrigger needed\n";
 snow_reset();ground(0,35);snow_step(.01,2.5,0);dir=tractors[0].direction;reversals=0;
 for(int k=0;k<5000;++k){ground(0,40);snow_step(.01,2.5,0);if(tractors[0].direction!=dir){++reversals;dir=tractors[0].direction;}}
 assert(reversals==2&&!tractors[0].active&&!cycles[0].armed);double parked=tractors[0].x;
 for(int k=0;k<1000;++k){ground(0,40);snow_step(.01,2.5,0);assert(tractors[0].x==parked&&!tractors[0].active);}
 std::cout<<"PASS sustained blizzard >80% cannot exceed two passes or immediately restart\n";
 ground(0,42*.6);snow_step(.01,0,0);assert(!cycles[0].armed);
 ground(0,42*.59);snow_step(.01,0,0);assert(cycles[0].armed&&!tractors[0].active);
 ground(0,42*.8);snow_step(.01,0,0);assert(!tractors[0].active);
 ground(0,42*.81);ground(1,42*.81);snow_step(.01,0,0);assert(tractors[0].active&&tractors[1].active);
 std::cout<<"PASS exact60 does not rearm; below60 then above80 rearms; per-display activation independent\n";
 // Different display lengths mean the second cycle finishes first.
 for(int k=0;k<1000;++k){ground(0,40);ground(1,40);snow_step(.01,0,0);}
 assert(tractors[0].active&&!tractors[1].active&&cycles[1].passes==2);
 std::cout<<"PASS per-display pass budgets independent\n";
}
