#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
#include <numeric>
#include <cstdlib>
struct Metrics {double vx,vy,radius,interval,origin,trajectory;size_t count;};
double sd(const std::vector<double>& a){double mean=std::accumulate(a.begin(),a.end(),0.0)/a.size(),v=0;for(double x:a)v+=(x-mean)*(x-mean);return std::sqrt(v/a.size());}
double cv(const std::vector<double>& a){return sd(a)/(std::accumulate(a.begin(),a.end(),0.0)/a.size());}
double material(){auto d=snow_diagnostics();return d.pileArea+d.pendingArea+d.airborneArea;}
Metrics sample(double dt){
 snow_reset();snow_forget_windows();snow_corner_inset(10);
 SnowRect display{0,0,1600,3000};snow_displays(&display,1);
 SnowWindow w{10,{500,300,320,100}};snow_windows(&w,1);
 auto &s=surfaces[10];std::fill(s.h.begin(),s.h.end(),42);double initial=material();
 std::vector<double> ux,uy,rad,gaps,origin,travel;double last=0;
 struct Birth{size_t index;int frame;double x;};std::vector<Birth> births;
 const int ageFrames=int(std::round(.1/dt));
 for(int frame=0;frame<int(1.2/dt);++frame){size_t old=clumps.size();snow_step(dt,0,0);
  assert(std::abs(material()-initial)<1e-7);assert(clumps.size()<=maxClumps);
  for(auto birth:births)if(frame-birth.frame==ageFrames)travel.push_back(clumps[birth.index].x-birth.x);
  for(size_t i=old;i<clumps.size();++i){auto p=clumps[i];if(p.x>=s.r.x)continue;
   // Slough emission occurs after integration; these are actual birth states.
   ux.push_back(p.vx);uy.push_back(p.vy);rad.push_back(p.r);origin.push_back(p.x+p.r-s.r.x);
   births.push_back({i,frame,p.x});
   double now=(frame+1)*dt;if(last>0)gaps.push_back(now-last);last=now;
   assert(p.vx>=-22&&p.vx<=-9&&p.vy>=-6&&p.vy<=8);
   assert(p.area>0&&p.area<=24&&std::abs(p.area-pi*p.r*p.r)<1e-10);
  }
 }
 assert(ux.size()>10&&gaps.size()>8);
 Metrics m{sd(ux),sd(uy),cv(rad),cv(gaps),sd(origin),sd(travel),ux.size()};
 std::cout<<"dt="<<dt<<" births="<<m.count<<" launch_vx_sd="<<m.vx<<" launch_vy_sd="<<m.vy<<" radius_cv="<<m.radius<<" interval_cv="<<m.interval<<" origin_sd="<<m.origin<<" age100ms_travel_sd="<<m.trajectory<<" conserved="<<material()<<"\n";
 return m;
}
void releaseSample(bool report){
 snow_reset();snow_forget_windows();
 // One 8-point band, 30 points deep: actual queued release at normal material scale.
 shedding.push_back({500,300,8,240,1200,0,0,10});
 std::vector<double> vx,vy,r,gaps;double last=0;
 for(int k=1;k<=300;++k){size_t old=clumps.size();shed(.001);
  double total=0;for(auto p:clumps)total+=p.area;for(auto b:shedding)total+=b.area;
  assert(std::abs(total-240)<1e-8&&clumps.size()<=maxClumps);
  for(size_t i=old;i<clumps.size();++i){auto p=clumps[i];vx.push_back(p.vx);vy.push_back(p.vy);r.push_back(p.r);
   if(last>0){gaps.push_back(k*.001-last);}last=k*.001;
   // v3.3: fracture impulse is relative to a freely falling patch, not a frozen mouth.
   assert(p.x>=500&&p.x<=508&&std::abs(p.vx)<=4&&p.vy-460*k*.001>=-5.00001&&p.vy-460*k*.001<=7.00001&&p.area<=32);
  }
 }
 assert(shedding.empty()&&vx.size()>5);
 std::cout<<"release births="<<vx.size()<<" vx_sd="<<sd(vx)<<" vy_sd="<<sd(vy)<<" radius_cv="<<cv(r)<<" interval_cv="<<cv(gaps)<<" conserved=240\n";
 if(!report)assert(sd(vx)>1&&sd(vy)>2&&cv(r)>.08&&cv(gaps)>.10);
}
int main(){bool report=std::getenv("SNOW_BASELINE")!=nullptr;
 for(double dt:{1.0/30,1.0/60,1.0/120}){auto a=sample(dt),b=sample(dt);
  assert(a.count==b.count&&a.vx==b.vx&&a.radius==b.radius&&a.interval==b.interval);
  if(!report){assert(a.vx>2&&a.vy>2&&a.radius>.12&&a.interval>.18&&a.origin>.2&&a.trajectory>.12);}
 }
 releaseSample(report);
 std::cout<<"PASS actual production edge births: reproducibility, bounded impulses, exact material area; diversity thresholds "<<(report?"report only":"enforced")<<"\n";
}
