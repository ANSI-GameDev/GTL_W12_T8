#pragma once

enum PhysicsType : int
{
    /** Follow owner. */
    PhysType_Default,
    /** Do not follow owner, but make kinematic. */
    PhysType_Kinematic,		
    /** Do not follow owner, but simulate. */
    PhysType_Simulated,
};
