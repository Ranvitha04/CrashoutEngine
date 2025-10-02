#pragma once

#include "math/CETypes.h"   
#include "math/CEVec2.h"     

namespace Patrol {

    // Using CEVec2 as float vector
    struct ABPatrol {
        CEVec2  pointA;           
        CEVec2  pointB;            
        CE::f32 speed;  
        
        CE::f32 dest;      
        bool towardsA;          

        //Example i guess
        ABPatrol()
            : pointA{ 0.f, 0.f }, pointB{ 100.f, 0.f }, speed(120.f), dest(4.f), towardsA(true) {
        }
    };

    //CE::f32& Tx(CEMtx33* m);
    //CE::f32& Ty(CEMtx33* m);

    //void UpdateAB(CEMtx33* transform, ABPatrol* data, CE::f32 dt);

} 