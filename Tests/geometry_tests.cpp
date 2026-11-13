#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
double mass(){auto d=snow_diagnostics();return d.pileArea+d.airborneArea+d.pendingArea;}
std::vector<SnowCurve> exported(){int n=snow_contours(nullptr,0);std::vector<SnowCurve> v(n);assert(snow_contours(v.data(),n)==n);return v;}
double cubic(double a,double b,double c,double d,double t){double u=1-t;return u*u*u*a+3*u*u*t*b+3*u*t*t*c+t*t*t*d;}
void check(const std::vector<SnowCurve>& curves,double left,double right,double expectedArea){
 double integral=0,lastX=0,lastY=0;int open=0;
 for(auto c:curves){
  assert(c.x0>=left-1e-9&&c.x3<=right+1e-9&&c.x3>c.x0);
  assert(c.x1>=c.x0&&c.x2>=c.x1&&c.x3>=c.x2);
  assert(c.y1==c.y0&&c.y2==c.y3); // zero tangent, including at caps
  if(c.start){assert(!open&&c.y0==c.baseY);open=1;}
  else assert(open&&std::abs(lastX-c.x0)<1e-9&&std::abs(lastY-c.y0)<1e-9);
  for(int i=0;i<=100;++i){double t=i/100.0,y=cubic(c.y0,c.y1,c.y2,c.y3,t);
   assert(y>=std::min(c.y0,c.y3)-1e-9&&y<=std::max(c.y0,c.y3)+1e-9);
  }
  integral+=(c.x3-c.x0)*(c.baseY-(c.y0+c.y3)/2);
  lastX=c.x3;lastY=c.y3;if(c.end){assert(c.y3==c.baseY);open=0;}
 }
 assert(!open&&std::abs(integral-expectedArea)<1e-7);
}
int main(){
 SnowRect d={0,0,640,1000};snow_displays(&d,1);
 SnowWindow w={10,{100,200,83,80}};snow_windows(&w,1);snow_reset();
 auto &s=surfaces[10];std::fill(s.h.begin(),s.h.end(),12);
 assert(s.r.x==110&&s.r.x+s.r.w==173);
 check(exported(),110,173,63*12);
 std::cout<<"PASS default rounded support excludes unsupported outer corners; area-exact C1 native cubic caps\n";
 for(double width:{.1,1.0,3.0,7.9,8.0,8.1,19.0,83.0,320.0}){
  snow_forget_windows();snow_corner_inset(0);w.rect.w=width;snow_windows(&w,1);
  auto &a=surfaces[10];for(size_t i=0;i<a.h.size();++i)a.h[i]=1+double((i*17)%42);
  check(exported(),w.rect.x,w.rect.x+width,mass());
  auto curves=exported();for(auto c:curves)for(int i=1;i<10;++i){double t=i/10.0;
   double x=cubic(c.x0,c.x1,c.x2,c.x3,t),y=cubic(c.y0,c.y1,c.y2,c.y3,t);
   size_t j=std::min(a.h.size()-1,(size_t)((x-a.r.x)/bin));
   assert(std::abs(contourTop(a,j,x)-y)<1e-8);
  }
 }
 std::cout<<"PASS production exported curve/collision agreement; monotone no-overshoot bounds incl subpoint partial bins\n";
 snow_forget_windows();w.rect.w=83;SnowWindow list[2]={{11,{133,100,1,150}},w};snow_windows(list,2);
 auto &a=surfaces[10];std::fill(a.h.begin(),a.h.end(),12);
 auto curves=exported();int starts=0;
 for(auto c:curves){starts+=c.start;assert(c.x3<=132||c.x0>=140);}
 assert(starts==2&&!a.visible[4]);check(curves,100,183,12*(83-8));
 double before=mass();w.rect.x+=30;list[1]=w;snow_windows(list,2);
 assert(std::abs(mass()-before)<1e-8);
 std::cout<<"PASS one-point occluder cannot be bridged by bin/render/collision; hidden accepted mass survives release\n";
 snow_forget_windows();SnowWindow pair[2]={{10,{100,200,40,50}},{11,{140,200,40,50}}};snow_windows(pair,2);
 for(auto &e:surfaces)if(e.first>=0)std::fill(e.second.h.begin(),e.second.h.end(),10);
 curves=exported();starts=0;for(auto c:curves)starts+=c.start;assert(starts==2);
 check(curves,100,180,800);
 std::cout<<"PASS adjacent equal-height windows remain two explicit native path runs\n";
 snow_forget_windows();w={10,{100,200,83,80}};snow_windows(&w,1);auto &b=surfaces[10];std::fill(b.h.begin(),b.h.end(),12);
 double x=b.r.x+.05,top=contourTop(b,0,x);assert(top>b.r.y-b.h[0]+5);
 flakes={{x,top-1,1,80,-.02}};double initial=mass();snow_step(.02,0,0);
 assert(flakes.empty()&&std::abs(mass()-initial-4)<1e-7);
 std::cout<<"PASS real flake collision uses tapered visible edge, not rectangular bin top\n";
 snow_reset();std::fill(b.h.begin(),b.h.end(),42);
 assert(b.r.y-contourTop(b,0,b.r.x+4)<10); // a ramp, not a near-vertical 42pt wall
 initial=mass();snow_step(.05,0,0);
 assert(std::abs(mass()-initial)<1e-8);for(auto c:clumps)assert(c.area<=24&& (c.x<b.r.x||c.x>b.r.x+b.r.w));
 Clump q{};initial=mass();assert(overflow(10,b,4000,q,b.h.size()-1));
 assert(q.area<=32&&shedding.back().width==4&&shedding.back().x==b.r.x+b.r.w);
 clumps.push_back(q);assert(std::abs(mass()-initial-4000)<1e-8);
 for(int i=0;i<60;++i){snow_step(.01,0,0);for(auto c:clumps)assert(c.area<=32);}
 assert(std::abs(mass()-initial-4000)<1e-6);
 std::cout<<"PASS edge slough/large overflow granulate at actual support ends with conserved pending and airborne area\n";
 snow_forget_windows();snow_corner_inset(10);w={10,{100,200,83,80}};snow_windows(&w,1);
 std::fill(surfaces[10].h.begin(),surfaces[10].h.end(),12);initial=mass();
 w.rect.w+=2;snow_windows(&w,1);assert(std::abs(mass()-initial)<1e-8);
 w.rect.w-=4;snow_windows(&w,1);assert(std::abs(mass()-initial)<1e-8);
 snow_corner_inset(0);snow_windows(&w,1);assert(surfaces[10].r.x==100&&std::abs(mass()-initial)<1e-8);
 std::cout<<"PASS partial-bin growth/shrink and rounded-to-square profile changes conserve area\n";
}
