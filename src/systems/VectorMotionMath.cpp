#include "nightwalker/systems/MotionImpulseMath.h"
#include <algorithm>
#include <cmath>
namespace nightwalker::systems::motion_impulse_math {
Plan BuildPlan(const game::Vec3& a,const game::Vec3& b,double h,double u,double meters) noexcept {
 const double dx=static_cast<double>(b.x)-a.x,dy=static_cast<double>(b.y)-a.y;
 const double len=std::sqrt(dx*dx+dy*dy);if(len<0.05)return {};
 const double nx=dx/len,ny=dy/len;
 h=std::clamp(h,0.0,3.0);u=std::clamp(u,0.0,1.0);meters=std::clamp(meters,0.5,3.5);
 Plan p{};p.valid=true;p.impulse={static_cast<float>(nx*h),static_cast<float>(ny*h),static_cast<float>(u)};
 p.projectedEnd={static_cast<float>(b.x+nx*meters),static_cast<float>(b.y+ny*meters),b.z};return p;
}
bool WithinRange(const game::Vec3&a,const game::Vec3&b,double maxDistance) noexcept {
 const double dx=static_cast<double>(a.x)-b.x,dy=static_cast<double>(a.y)-b.y,dz=static_cast<double>(a.z)-b.z;
 maxDistance=std::max(0.0,maxDistance);return dx*dx+dy*dy+dz*dz<=maxDistance*maxDistance;
}
}
