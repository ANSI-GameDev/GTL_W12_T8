#include  "PhysicsConstraintTemplate.h"

void UPhysicsConstraintTemplate::SetDefaultProfile(FConstraintInstance& CI)
{
    DefaultProfile = CI.ProfileInstance;
}

void UPhysicsConstraintTemplate::Serialize(FArchive& Ar)
{
    FConstraintProfileProperties CurrentProfile = DefaultInstance.ProfileInstance;

    DefaultInstance.ProfileInstance = CurrentProfile;


}
