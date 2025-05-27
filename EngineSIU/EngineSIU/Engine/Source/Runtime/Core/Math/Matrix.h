#pragma once
#include "Axis.h"
#include "MathFwd.h"
#include "MathUtility.h"
#include "Serialization/Archive.h"

// 4x4 행렬 연산
struct alignas(16) FMatrix
{
public:
    alignas(16) float M[4][4];

    using FReal = float;

public:
    static const FMatrix Identity;

public:
    // 기본 연산자 오버로딩
    FMatrix operator+(const FMatrix& Other) const;
    FMatrix operator-(const FMatrix& Other) const;
    FMatrix operator*(const FMatrix& Other) const;
    FMatrix& operator*=(const FMatrix& Other);
    FMatrix operator*(float Scalar) const;
    FMatrix operator/(float Scalar) const;
    float* operator[](int row);
    const float* operator[](int row) const;

    FVector ExtractScaling(float Tolerance = SMALL_NUMBER);
    FVector GetOrigin() const;
    void SetOrigin(const FVector& NewOrigin);
    FVector4 GetColumn(int32 ColumnIndex) const;
    float Determinant() const;

    void SetAxis(int32 i, const FVector& Axis);
    FVector GetScaledAxis(EAxis::Type InAxis) const;

    // 유틸리티 함수
    static FMatrix Transpose(const FMatrix& Mat);
    static FMatrix Inverse(const FMatrix& Mat);
    static FMatrix CreateTranslationMatrix(const FVector& V);
    static FMatrix CreateRotationMatrix(const FRotator& R);
    static FMatrix CreateRotationMatrix(const FQuat& Q);
    static FMatrix CreateScaleMatrix(const FVector& V);

    static FVector TransformVector(const FVector& V, const FMatrix& M);
    static FVector4 TransformVector(const FVector4& V, const FMatrix& M);

    FVector4 TransformFVector4(const FVector4& vector) const;
    FVector TransformPosition(const FVector& vector) const;

    FQuat ToQuat() const;

    FVector GetScaleVector(float Tolerance = SMALL_NUMBER) const;

    FVector GetTranslationVector() const;

    FMatrix GetMatrixWithoutScale(float Tolerance = SMALL_NUMBER) const;

    FVector4 TransformVector(const FVector& V) const;

    /**
     *	이 매트릭스의 역행렬로 방향 벡터를 변환합니다. (위치 정보는 무시됨)
     *	표면 노멀(또는 평면)을 변환하고 비균일 스케일링까지 올바르게 적용하려면
     *	행렬 역행렬의 adjoint를 사용하는 TransformByUsingAdjointT를 사용하세요.
     */
    FVector InverseTransformVector(const FVector& V) const;

    void RemoveScaling(float Tolerance = SMALL_NUMBER);

    bool Equals(const FMatrix& Other, float Tolerance = KINDA_SMALL_NUMBER) const;
    static FMatrix MakeLookAtLH(const FVector& Eye, const FVector& Target, const FVector& Up);
};

namespace FRotationMatrix
{
    inline FQuat MakeFromXY(const FVector& XAxis, const FVector& YAxis);
}

inline FArchive& operator<<(FArchive& Ar, FMatrix& M)
{
    Ar << M.M[0][0] << M.M[0][1] << M.M[0][2] << M.M[0][3];
    Ar << M.M[1][0] << M.M[1][1] << M.M[1][2] << M.M[1][3];
    Ar << M.M[2][0] << M.M[2][1] << M.M[2][2] << M.M[2][3];
    Ar << M.M[3][0] << M.M[3][1] << M.M[3][2] << M.M[3][3];
    return Ar;
}
