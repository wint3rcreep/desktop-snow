// Production snowfall from empty supports, not an edge-height emission fixture.
#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
#include <cstdlib>
double mass(){double a=0;for(auto const &e:surfaces)for(size_t i=0;i<e.second.h.size();++i)a+=e.second.h[i]*std::min(bin,e.second.r.w-i*bin);for(auto c:clumps)a+=c.area;return a;}
int main(){
 snow_corner_inset(std::getenv("SNOW_ROUNDED")?10:0); // Both profiles run.
 SnowRect d={0,0,640,600};SnowWindow w[3]={{10,{240,100,80,80}},{20,{200,260,160,80}},{30,{160,420,240,80}}};
 snow_displays(&d,1);snow_windows(w,3);snow_reset();snow_tractor_enabled(0);rng.seed(9271);
 int upper=0,middle=0,lower=0;
 for(int k=0;k<36000;++k){
  snow_step(1.0/60,1,0);
  if(k%7==0)snow_windows(w,3);
  if(k>=32400)for(auto c:clumps){upper+=c.source==10;middle+=c.source==20;lower+=c.source==30;}
  for(auto const &e:surfaces)for(double h:e.second.h)assert(h>=0&&h<=42.000001);
  assert(clumps.size()<=maxClumps&&flakes.size()<=7000);
 }
 std::cout<<"normal intensity=1, 600s,60Hz: clump observations in LAST 60s upper="<<upper<<" middle="<<middle<<" lower="<<lower<<std::endl;
 assert(upper>0&&middle>0&&lower>0);
 std::cout<<"PASS mature normal accumulation continues cascading through all three tiers instead of silently clipping at42\n";
 // Fresh normal accumulation; stop incoming flakes to isolate already accepted mass.
 // Clearing air here is not an edge preload; all pile and clump heights are naturally reached.
 snow_reset();rng.seed(9271);
 for(int k=0;k<3600;++k)snow_step(1.0/60,1,0);
 flakes.clear();double initial=mass();for(int k=0;k<600;++k)snow_step(1.0/60,0,0);
 std::cout<<"accepted snow conservation after natural60s: delta="<<mass()-initial<<std::endl;
 assert(std::abs(mass()-initial)<1e-6);
 std::cout<<"PASS naturally accumulated accepted mass conserved through unsaturated recipients\n";
 // Follow a clump produced by normal accumulation, not release()/scatter() or a fixture.
 auto followNatural=[&](bool lower){
  snow_forget_windows();snow_displays(&d,1);snow_windows(w,lower?2:1);snow_reset();rng.seed(9271);
  for(int k=0;k<5000&&std::none_of(clumps.begin(),clumps.end(),[](const Clump &c){return c.source==10;});++k)snow_step(1.0/60,1,0);
  auto it=std::find_if(clumps.begin(),clumps.end(),[](const Clump &c){return c.source==10;});
  assert(it!=clumps.end());Clump natural=*it;
  flakes.clear();clumps={natural}; // isolate that real emitted parcel
  double targetY=lower?260:599,targetBefore=0;
  auto targetArea=[&](){double a=0;for(const auto &e:surfaces)if(std::abs(e.second.r.y-targetY)<.01)for(size_t i=0;i<e.second.h.size();++i)a+=e.second.h[i]*std::min(bin,e.second.r.w-i*bin);return a;};
  // Existing banks remain untouched; the whole accepted-mass transfer is checked.
  // Compare with an otherwise identical no-parcel control: recipient may also
  // shed during these four seconds, so raw net growth is not a receipt test.
  auto savedSurfaces=surfaces;auto savedClumps=clumps;auto savedShedding=shedding;
  clumps.clear();
  for(int k=0;k<240;++k)snow_step(1.0/60,0,0);
  targetBefore=targetArea();
  surfaces=savedSurfaces;clumps=savedClumps;shedding=savedShedding;
  double before=mass();
  for(int k=0;k<240;++k)snow_step(1.0/60,0,0);
  std::cerr<<"cascade recipient="<<lower<<" initial parcel="<<natural.area<<" target delta="<<targetArea()-targetBefore<<" mass delta="<<mass()-before<<"\n";
  assert(targetArea()>targetBefore&&std::abs(mass()-before)<1e-6);
 };
 followNatural(true);std::cout<<"PASS naturally emitted edge parcel reaches lower window; accepted area conserved\n";
 followNatural(false);std::cout<<"PASS no lower recipient at edge: naturally emitted parcel reaches floor\n";
 // Partial overlap: the upper front window masks the left part of the lower top;
 // the visible extension must still accept edge snow; no spill across occlusion gaps.
 w[1].rect={280,150,83,150};
 snow_forget_windows();snow_displays(&d,1);snow_windows(w,2);snow_reset();rng.seed(9271);
 for(int k=0;k<7200;++k){snow_step(.0166666666666667,1,0);if(k%7==0)snow_windows(w,2);}
 assert(!surfaces[20].visible[0]&&surfaces[20].visible.back());
 bool partialSnow=false;for(auto p:surfaces[20].h)partialSnow|=p>0;assert(partialSnow);
 std::cout<<"PASS partial overlapping windows and three-point end bin retain visible lower accumulation\n";
 for(double dt:{.01,.05}){
  snow_forget_windows();snow_displays(&d,1);snow_windows(w,1);snow_reset();rng.seed(9271);int seen=0;
  for(int k=0;k<int(200/dt);++k){snow_step(dt,1,0);if(k>int(190/dt))seen+=!clumps.empty();for(double h:surfaces[10].h)assert(h>=0&&h<=42.000001);}
  assert(seen>0);
 }
 std::cout<<"PASS continuous normal spill at dt10ms and50ms remains bounded\n";
}
