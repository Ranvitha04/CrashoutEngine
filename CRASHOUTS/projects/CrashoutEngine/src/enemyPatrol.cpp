#include "enemyPatrol.h"
#include <cmath> 
namespace Patrol {

    CE::f32& Tx(CEMtx33* m) { return m->m[0][2]; } 
    CE::f32& Ty(CEMtx33* m) { return m->m[1][2]; }

    void UpdateAB(CEMtx33* transform, ABPatrol* data, CE::f32 dt)
    {
        if (transform == 0 || data == 0) return;
        CE::f32 tx, ty;

        if (data->towardsA) 
        {
            tx = data->pointA.x; ty = data->pointA.y; 
        }
        else 
        { 
            tx = data->pointB.x; ty = data->pointB.y; 
        }

        CE::f32 px = Tx(transform);
        CE::f32 py = Ty(transform);
        CE::f32 dx = tx - px;
        CE::f32 dy = ty - py;
        CE::f32 d2 = dx * dx + dy * dy;
        CE::f32 thr2 = data->dest * data->dest;

        if (d2 <= thr2) {
            data->towardsA = !data->towardsA; 
            return;                           
        }
        CE::f32 step = data->speed * dt;
        if (dx > 0.f) 
        {
            px += step; if (px > tx) px = tx; 
        }
        else if (dx < 0.f) 
        { 
            px -= step; if (px < tx) px = tx; 
        }

        if (dy > 0.f)
        { 
            py += step; if (py > ty) py = ty;
        }
        else if (dy < 0.f) 
        { 
            py -= step; if (py < ty) py = ty; 
        }

    }

} 