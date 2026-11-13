#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
double bez(double a,double b,double c,double d,double t){double u=1-t;return u*u*u*a+3*u*u*t*b+3*u*t*t*c+t*t*t*d;}
double renderedTop(Shedding b,double x){
 auto saved=shedding;shedding={b};int n=snow_contours(nullptr,0);std::vector<SnowCurve> curves(n);snow_contours(curves.data(),n);shedding=saved;
 for(auto c:curves)if(c.baseY==b.bottom&&x>=c.x0&&x<=c.x3){double lo=0,hi=1;
  for(int j=0;j<60;++j){double t=(lo+hi)/2;if(bez(c.x0,c.x1,c.x2,c.x3,t)<x)lo=t;else hi=t;}
  return bez(c.y0,c.y1,c.y2,c.y3,(lo+hi)/2);
 }assert(false);return 0;
}
int main(){
 SnowRect d={-800,-300,1600,1800};snow_displays(&d,1);snow_corner_inset(10);
 SnowWindow w={9,{-300,-100,83,100}};snow_windows_at(&w,1,1);snow_reset();
 auto &s=surfaces[9];std::fill(s.h.begin(),s.h.end(),42);
 Clump q{};assert(overflow(9,s,20,q,0));
 // Exported Bezier endpoint is on baseY, not rectangular storage height.
 SnowCurve curves[100];int n=snow_contours(curves,100);bool endpoint=false;
 for(int i=0;i<n;++i)if(curves[i].x0==s.r.x){assert(curves[i].y0==s.r.y);endpoint=true;}
 assert(endpoint);std::cout<<"anchor gap="<<s.r.y-(q.y+q.r)<<std::endl;
 assert(std::abs(q.y+q.r-s.r.y)<1e-8);
 double initial=snow_diagnostics().pileArea;w.rect.x+=100;snow_windows_at(&w,1,1.1);
 assert(!shedding.empty());double bottom=shedding[0].bottom;shed(.01);
 assert(shedding[0].bottom>bottom);double first=shedding[0].bottom;shed(.01);
 assert(shedding[0].bottom-first>first-bottom);
 double mass=0;for(auto b:shedding)mass+=b.area;for(auto p:clumps)mass+=p.area;
 assert(std::abs(mass-initial)<1e-7);
 std::cout<<"PASS actual exported edge anchor; unsupported patches accelerate immediately; mass conserved\n";
 for(double dt:{1.0/30,1.0/60,1.0/120}){
  snow_forget_windows();snow_reset();
  // Backpressure cannot pin material in space; exact ballistic patch drift.
  shedding.push_back({-200,-100,8,240,1,10,30,9});
  for(int i=0;i<int(std::round(.5/dt));++i)shed(dt);
  assert(shedding.size()==1);auto b=shedding[0];
  assert(std::abs(b.bottom-(-100+230*.25))<1e-8);
  assert(std::abs(b.x-(-200+30*(1-std::exp(-.75))/1.5))<1e-8);
  assert(std::abs(b.area-240)<1e-8);
  // Fracture births use the CURRENT falling patch, with its velocity.
  shedding[0].delay=0;shedding[0].rate=2400;size_t old=clumps.size();shed(.02);
  auto moving=shedding[0];double remaining=240;
  assert(clumps.size()>old);
  for(size_t j=old;j<clumps.size();++j){auto grain=clumps[j];
   assert(grain.x>=moving.x&&grain.x<=moving.x+8);
   moving.area=remaining;
   assert(std::abs(grain.y-grain.r-renderedTop(moving,grain.x))<1e-8);
   assert(grain.vy>=moving.vy-5&&grain.vy<=moving.vy+7);remaining-=grain.area;
  }
  std::cout<<"PASS ballistic pending mass/drift and moving fracture origin dt="<<dt<<"\n";
 }
 // A whole patch may reach a lower support before its staged breakup completes.
 snow_forget_windows();snow_reset();SnowWindow lower={11,{-400,0,300,100}};
 snow_windows(&lower,1);shedding.push_back({-200,-10,8,240,1,10,0,9});
 for(int i=0;i<30;++i)shed(.01);
 assert(shedding.empty());assert(std::abs(snow_diagnostics().pileArea-240)<1e-8);
 std::cout<<"PASS lower support captures unbroken falling patch before fracture\n";
}
