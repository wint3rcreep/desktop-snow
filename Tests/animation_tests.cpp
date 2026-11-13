#include "../Core/Snow.cpp"
#include <cassert>
#include <iostream>
int main(){
 SnowRect d={-640,-480,640,480};snow_displays(&d,1);snow_windows(nullptr,0);snow_reset();
 balls={{-300,-50,48}};for(auto &e:surfaces)std::fill(e.second.h.begin(),e.second.h.end(),20);
 double before=snow_diagnostics().pileArea; snow_down(-300,-50,0);snow_drag(-280,-1,0);
 assert(std::abs(snow_diagnostics().pileArea-before)<1e-8);
 std::cout<<"PASS capped snowball leaves uncollected area on support\n";
 snow_up();snow_reset();balls={{-300,-24,20,0,220}};
 snow_step(.03,0,0);auto pose=snow_ball_pose(balls[0].x,balls[0].y);
 assert(pose.squash>0 && pose.squash<=.12);assert(balls[0].y+balls[0].r<=-1+1e-9);
 std::cout<<"PASS bounded cosmetic landing squash keeps physical bottom and radius\n";
 for(int i=0;i<600;++i)snow_step(.01,0,0);
 pose=snow_ball_pose(balls[0].x,balls[0].y);assert(std::abs(pose.squash)<1e-7);
 std::cout<<"PASS damped landing returns to rest without repeated contact excitation\n";
 snow_reset();balls={{-300,-31,30},{-400,-70,20},{-500,-100,12}};
 snow_down(-400,-70,0);snow_drag(-300,-79,0);snow_up();
 snow_down(-500,-100,0);snow_drag(-300,-109,0);snow_up();snow_step(.016,0,0);
 pose=snow_ball_pose(balls[2].x,balls[2].y);assert(std::abs(pose.tilt)>1e-5);
 std::cout<<"PASS stacking excites bounded assembled settling pose\n";
 for(int i=0;i<700;++i)snow_step(.01,0,0);
 assert(std::abs(snow_ball_pose(balls[2].x,balls[2].y).tilt)<1e-7);
 snow_down(balls[0].x,balls[0].y,0);snow_drag(-250,-31,0);
 assert(balls[2].x==balls[0].x);snow_up();snow_step(.016,0,0);
 assert(std::abs(snow_ball_pose(balls[2].x,balls[2].y).tilt)>1e-5);
 std::cout<<"PASS base drag moves live attachments even without step and excites settle\n";
 auto frozen=snow_ball_pose(balls[2].x,balls[2].y);auto bx=balls[0].x;
 snow_step(0,0,100);snow_step(-1,0,100);snow_step(NAN,0,100);
 for(int i=0;i<10;++i){auto q=snow_ball_pose(balls[2].x,balls[2].y);assert(q.tilt==frozen.tilt&&q.flutter==frozen.flutter);}
 assert(balls[0].x==bx);
 std::cout<<"PASS paused invalid dt and repeated redraw exports never advance animation\n";
 for(int i=0;i<1000;++i){snow_step(.05,0,100);auto q=snow_ball_pose(balls[2].x,balls[2].y);assert(std::isfinite(q.tilt)&&std::abs(q.tilt)<=.055&&std::abs(q.flutter)<=.10);}
 assert(std::abs(snow_ball_pose(balls[2].x,balls[2].y).flutter)>.01);
 std::cout<<"PASS bounded deterministic wind equilibrium on negative-coordinate display\n";
 snow_down(balls[2].x,balls[2].y,0);snow_drag(-500,-150,0);snow_up();
 SnowSprite out[32];int n=snow_sprites(out,32);for(int i=0;i<n;++i){assert(out[i].kind==1);auto q=snow_ball_pose(out[i].x,out[i].y);assert(q.tilt==0&&q.flutter==0);}
 assert(snow_ball_pose(-999,-999).squash==0);
 std::cout<<"PASS breakup removes all accessory motion and roles immediately\n";
 snow_reset();balls={{-400,-200,20,90,0}};double x=balls[0].x,a=balls[0].angle;
 snow_step(.03,0,0);assert(std::abs((balls[0].angle-a)-(balls[0].x-x)/20)<1e-12);
 snow_down(balls[0].x,balls[0].y,0);x=balls[0].x;a=balls[0].angle;snow_drag(x+40,balls[0].y,0);
 assert(std::abs((balls[0].angle-a)-2)<1e-12);
 std::cout<<"PASS signed rolling angle equals actual horizontal travel divided by radius in step and drag\n";
 snow_up();snow_reset();balls={{-300,-20,47}};for(auto &e:surfaces)std::fill(e.second.h.begin(),e.second.h.end(),20);
 before=snow_diagnostics().pileArea+pi*balls[0].r*balls[0].r;snow_down(-300,-20,0);snow_drag(-280,-1,0);
 assert(std::abs(snow_diagnostics().pileArea+pi*balls[0].r*balls[0].r-before)<1e-8);
 assert(balls[0].r<=48);
 std::cout<<"PASS growing snowball conserves 2D area including radius-cap crossing\n";
 snow_up();snow_reset();balls={{-300,-100,20,100,220}};snow_step(100,0,0);auto large=balls[0];
 snow_reset();balls={{-300,-100,20,100,220}};snow_step(.05,0,0);
 assert(balls[0].x==large.x&&balls[0].y==large.y&&balls[0].squash==large.squash);
 std::cout<<"PASS long sleep clamps physics and animation dt together\n";
}

