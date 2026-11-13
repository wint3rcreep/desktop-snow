#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
double pileMass(const Surface&s){double a=0;for(size_t i=0;i<s.h.size();++i)a+=s.h[i]*std::min(bin,s.r.w-i*bin);return a;}
double energy(const Surface&s){double a=0;for(size_t i=0;i<s.h.size();++i)a+=s.h[i]*s.h[i]*std::min(bin,s.r.w-i*bin);return a;}
int main(){snow_corner_inset(0);SnowRect d={0,0,2000,1000};snow_displays(&d,1);SnowWindow w={90,{100,200,1280,80}};snow_windows(&w,1);snow_reset();auto &s=surfaces[90];for(size_t i=4;i+4<s.h.size();++i)s.h[i]=(i%2)?20:2;auto before=s.h;double area=pileMass(s);snow_step(1.0/60,0,0);int changed=0;for(size_t i=0;i<s.h.size();++i)changed+=std::abs(s.h[i]-before[i])>1e-9;assert(changed<=8&&changed>0);assert(std::abs(pileMass(s)-area)<1e-8);std::cout<<"PASS localized avalanche changes="<<changed<<" conserved area="<<area<<"\n";
 snow_reset();s.h[40]=40;area=pileMass(s);double previous=energy(s);for(int i=0;i<6000;++i){snow_step(1.0/120,0,0);assert(energy(s)<=previous+1e-8);previous=energy(s);assert(std::abs(pileMass(s)-area)<1e-7);assert(s.avalanche.size()<=32);}assert(s.h[38]>0&&s.h[42]>0&&s.h[70]==0);assert(s.avalanche.empty());auto settled=s.h;for(int i=0;i<120;++i)snow_step(1.0/120,0,0);assert(s.h==settled);std::cout<<"PASS neighbor propagation stays local; potential decreases; front terminates without oscillation\n";
 std::vector<double> reference;for(int hz:{30,60,120}){snow_reset();s.h[40]=40;for(int i=0;i<2*hz;++i)snow_step(1.0/hz,0,0);assert(std::abs(pileMass(s)-320)<1e-8);if(reference.empty())reference=s.h;else for(size_t i=0;i<reference.size();++i)assert(std::abs(reference[i]-s.h[i])<1e-8);std::cout<<"PASS dt "<<hz<<"Hz same bounded local fracture evolution and mass\n";}
 snow_reset();s.h[40]=30;s.visible[41]=false;for(int i=0;i<120;++i)snow_step(1.0/60,0,0);assert(s.h[42]==0&&s.h[41]==0);std::cout<<"PASS local fracture never crosses an occluded gap\n";
 snow_reset();for(size_t i=1;i<s.h.size();++i)s.h[i]=i%2?8:2;auto stable=s.h;for(int i=0;i<120;++i)snow_step(1.0/60,0,0);assert(s.h==stable);std::cout<<"PASS below-ignition slope does not trigger whole-pile relaxation\n";
}
