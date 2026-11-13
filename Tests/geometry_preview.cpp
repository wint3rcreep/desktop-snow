// Deterministic documentation preview from the actual native-render export.
// This is a simulated geometry fixture, NOT a screenshot or macOS validation.
#include "../Core/Snow.cpp"
#include <iostream>
#include <iomanip>
int main(){
 snow_forget_windows();snow_displays(nullptr,0);snow_corner_inset(10);
 SnowWindow w[]={{99,{230,300,8,160}},{10,{100,100,320,80}},{20,{100,240,83,50}},{30,{100,380,320,80}}};
 snow_windows(w,4);
 for(auto &e:surfaces)if(e.first!=99)for(size_t i=0;i<e.second.h.size();++i)e.second.h[i]=24+4*std::sin(i*.45);
 std::cout<<std::fixed<<std::setprecision(5);
 std::cout<<"<svg xmlns='http://www.w3.org/2000/svg' width='640' height='540' viewBox='0 0 640 540'><rect width='640' height='540' fill='#142033'/><g fill='#ccd6e5' font-family='sans-serif' font-size='14'><text x='20' y='25'>SIMULATED production geometry — NOT macOS capture</text><text x='20' y='50'>10pt assumed shelf inset; exact exported native Bezier paths</text><text x='440' y='140'>deep bank</text><text x='210' y='270'>partial last bin</text><text x='440' y='420'>occluded gap</text></g>";
 for(int i=1;i<4;++i){auto r=w[i].rect;std::cout<<"<rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' rx='10' fill='#314660'/>";}
 auto r=w[0].rect;std::cout<<"<rect x='"<<r.x<<"' y='"<<r.y<<"' width='"<<r.w<<"' height='"<<r.h<<"' fill='#84545f'/>";
 int count=snow_contours(nullptr,0);std::vector<SnowCurve> c(count);snow_contours(c.data(),count);double x=0;
 for(auto v:c){
  if(v.start){x=v.x0;std::cout<<"<path fill='#f0f7ff' d='M "<<v.x0<<" "<<v.y0;}
  std::cout<<" C "<<v.x1<<" "<<v.y1<<" "<<v.x2<<" "<<v.y2<<" "<<v.x3<<" "<<v.y3;
  if(v.end)std::cout<<" L "<<x<<" "<<v.baseY<<" Z'/>";
 }
 std::cout<<"<text x='20' y='515' fill='#ccd6e5' font-family='sans-serif' font-size='13'>Curve shape only; no AppKit, pixel masking, dynamics or FPS validation.</text></svg>\n";
}
