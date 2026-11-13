#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
void fill(double h){for(auto &s:surfaces)std::fill(s.second.h.begin(),s.second.h.end(),h);}
void setup(){snow_tractor_enabled(0);SnowRect d={-960,-480,640,480};snow_displays(&d,1);snow_windows(nullptr,0);snow_reset();tractors[0]={-920,-1,1,0,0,0};snow_tractor_enabled(1);fill(38);}
int main(){
 setup();double previous=0;bool braking=false,turning=false;double lastHeading=1;
 for(int i=0;i<1300;++i){
  fill(38);auto old=tractors[0];double speed=cycles[0].speed;
  snow_step(.01,0,0);const auto &t=tractors[0];const auto &c=cycles[0];
  assert(t.x>=-920&&t.x<=-360);assert(c.speed>=0&&c.speed<=70);
  double delta=std::remainder(t.angle-old.angle,2*pi);
  assert(std::abs(delta-(t.x-old.x)/10)<1e-10);
  assert(c.speed-speed<=1.4+1e-9);assert(speed-c.speed<=1.8+1e-8);if(c.speed<speed)braking=true;
  if(c.turnRemaining>0){assert(t.x==previous || c.turnRemaining==.45);if(c.turnRemaining<.45)assert(std::abs(c.heading-lastHeading)<.08);turning=true;}
  previous=t.x;lastHeading=c.heading;
 }
 assert(braking&&turning);
 std::cout<<"PASS wheel radians equal signed actual displacement / rendered 10pt radius\n";
 std::cout<<"PASS bounded acceleration braking speed and endpoint travel on negative monitor\n";
 std::cout<<"PASS stationary dt-based smooth turn counts only one endpoint pass\n";
 auto old=tractors[0];auto c=cycles[0];
 for(double dt:{0.,-1.,double(NAN)})snow_step(dt,2.5,100);
 for(int i=0;i<100;++i){auto p=snow_tractor_pose(old.x,old.y);assert(p.speed==c.speed&&p.heading==c.heading&&p.lift==c.lift);}
 assert(tractors[0].x==old.x&&tractors[0].angle==old.angle&&cycles[0].turnRemaining==c.turnRemaining);
 std::cout<<"PASS zero invalid dt and repeated render exports freeze tractor animation\n";
 setup();for(size_t i=0;i<surfaces[-1].h.size();++i)surfaces[-1].h[i]=i%2?42:30;
 for(int i=0;i<100;++i){snow_step(.01,0,0);assert(cycles[0].lift>=0&&cycles[0].lift<=2.5);assert(std::abs(cycles[0].pitch)<=.045);}
 snow_reset();fill(0);tractors[0].active=1;tractors[0].cargo=1;cycles[0].lift=2.5;cycles[0].pitch=.045;
 for(int i=0;i<120;++i){fill(0);snow_step(.01,0,0);}
 assert(cycles[0].lift<.0001&&std::abs(cycles[0].pitch)<.00001);
 std::cout<<"PASS bounded overdamped suspension follows ground and decays without oscillation\n";
 setup();snow_step(.01,0,0);flakes.clear();double before=snow_diagnostics().pileArea+snow_diagnostics().airborneArea+tractors[0].cargo;
 snow_step(.01,0,0);auto d=snow_diagnostics();double after=d.pileArea+d.airborneArea+tractors[0].cargo+cycles[0].hauledArea;
 assert(std::abs(after-before)<1e-6);
 std::cout<<"PASS pickup scatter and single-count cargo conserve 2D material area\n";
 auto &t=tractors[0];auto &cycle=cycles[0];t.x=-360;t.cargo=123;double hauled=cycle.hauledArea;snow_step(.01,0,0);
 assert(t.cargo==0&&cycle.hauledArea==hauled+123&&cycle.passes==1);
 for(int i=0;i<3000;++i){fill(40);snow_step(.01,2.5,0);}
 assert(cycle.passes==2&&!t.active&&t.cargo==0&&!cycle.armed&&cycle.speed==0);
 auto parked=t;for(int i=0;i<500;++i){fill(40);snow_step(.01,2.5,0);assert(t.x==parked.x&&t.angle==parked.angle&&cycle.passes==2);}
 std::cout<<"PASS endpoint hauloff explicitly accounted; sustained blizzard parks at two passes with no cargo\n";
 double duration=0;
 for(double dt:{1./30,1./60,1./120}){
  setup();double elapsed=0;
  while(elapsed<30){fill(40);double v=cycles[0].speed;snow_step(dt,0,0);assert(cycles[0].speed-v<=140*dt+1e-7&&v-cycles[0].speed<=180*dt+1e-7);elapsed+=dt;if(!tractors[0].active)break;}
  assert(cycles[0].passes==2&&elapsed<20);
  if(duration){assert(std::abs(elapsed-duration)<.15);}duration=elapsed;
 }
 std::cout<<"PASS 30 60 120Hz bounded cycle durations agree without added passes\n";
 SnowRect disabled={-960,-480,640,480};snow_disabled_displays(&disabled,1);snow_displays(nullptr,0);
 for(int i=0;i<100;++i)snow_step(.01,2.5,50);
 SnowTractor out[2];assert(snow_tractors(out,2)==0&&tractors.empty());
 std::cout<<"PASS all monitors off removes tractor and remains idle\n";
}
