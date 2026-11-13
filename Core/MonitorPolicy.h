#pragma once
#include "Snow.h"
#include <vector>
#include <algorithm>
inline bool monitorPoint(const std::vector<SnowRect>& ds,double x,double y,double topMargin=0){
 for(auto d:ds)if(x>=d.x&&x<d.x+d.w&&y>=d.y-topMargin&&y<d.y+d.h)return true;
 return false;
}
inline bool monitorSame(SnowRect a,SnowRect b){return a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h;}

// Conservative interval intersection: partial bins touching disabled pixels are
// removed too. Unrelated offscreen trajectories retain baseline behavior.
inline bool monitorBlockedShelf(const std::vector<SnowRect>& ds,double x,double y,double width){
 for(auto d:ds)if(x<d.x+d.w&&x+width>d.x&&y>=d.y&&y<d.y+d.h)return true;
 return false;
}
