﻿#pragma once
#include "DistributionVector.h"


class UDistributionVectorUniform : public UDistributionVector
{
    DECLARE_CLASS(UDistributionVectorUniform, UDistributionVector)

public:
    UDistributionVectorUniform() = default;
    virtual ~UDistributionVectorUniform() override = default;

public:
    /** Upper end of FVector magnitude range. */
    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        FVector, Max, = FVector::OneVector;
    )

    /** Lower end of FVector magnitude range. */
    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        FVector, Min, = FVector::OneVector;
    )

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        EDistributionVectorLockFlags, LockedAxes, = EDistributionVectorLockFlags::None;
    )

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        bool, bUseExtremes, = false;
    )

public:
    //~ Begin UObject Interface
    virtual void PostInitProperties() override;
    // virtual void PostLoad() override;
    //~ End UObject Interface

    virtual FVector GetValue(float F = 0.f, UObject* Data = nullptr, int32 LastExtreme = 0, FRandomStream* InRandomStream = nullptr) const override;

    //Begin UDistributionVector Interface

    //@todo.CONSOLE: Currently, consoles need this? At least until we have some sort of cooking/packaging step!
    virtual ERawDistributionOperation GetOperation() const override;
    virtual uint8 GetLockFlag() const override;
    virtual uint32 InitializeRawEntry(float Time, float* Values) const override;
    virtual void GetRange(FVector& OutMin, FVector& OutMax) const override;
    //End UDistributionVector Interface

    /** These two functions will retrieve the Min/Max values respecting the Locked and Mirror flags. */
    virtual FVector GetMinValue() const;
    virtual FVector GetMaxValue() const;

    // We have 6 subs, 3 mins and three maxes. They are assigned as:
    // 0,1 = min/max x
    // 2,3 = min/max y
    // 4,5 = min/max z

    //~ Begin FCurveEdInterface Interface
    virtual int32 GetNumKeys() const override;
    virtual int32 GetNumSubCurves() const override;
    virtual FColor GetSubCurveButtonColor(int32 SubCurveIndex, bool bIsSubCurveHidden) const override;
    virtual float GetKeyIn(int32 KeyIndex) override;
    virtual float GetKeyOut(int32 SubIndex, int32 KeyIndex) override;
    virtual FColor GetKeyColor(int32 SubIndex, int32 KeyIndex, const FColor& CurveColor) override;
    virtual void GetInRange(float& MinIn, float& MaxIn) const override;
    virtual void GetOutRange(float& MinOut, float& MaxOut) const override;
    // virtual EInterpCurveMode GetKeyInterpMode(int32 KeyIndex) const override;
    virtual void GetTangents(int32 SubIndex, int32 KeyIndex, float& ArriveTangent, float& LeaveTangent) const override;
    virtual float EvalSub(int32 SubIndex, float InVal) override;
    virtual int32 CreateNewKey(float KeyIn) override;
    virtual void DeleteKey(int32 KeyIndex) override;
    virtual int32 SetKeyIn(int32 KeyIndex, float NewInVal) override;
    virtual void SetKeyOut(int32 SubIndex, int32 KeyIndex, float NewOutVal) override;
    // virtual void SetKeyInterpMode(int32 KeyIndex, EInterpCurveMode NewMode) override;
    virtual void SetTangents(int32 SubIndex, int32 KeyIndex, float ArriveTangent, float LeaveTangent) override;
    //~ Begin FCurveEdInterface Interface
};
