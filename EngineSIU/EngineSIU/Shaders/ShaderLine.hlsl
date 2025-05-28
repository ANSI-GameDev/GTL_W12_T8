
#include "ShaderRegisters.hlsl"

#define PI 3.14159265359
cbuffer GridParametersData : register(b1)
{
    float GridSpacing;
    int GridCount; // 총 grid 라인 수
    float2 Padding1;
    
    float3 GridOrigin; // Grid의 중심
    float Padding;
};

cbuffer PrimitiveCounts : register(b3)
{
    int BoundingBoxCount;
    int SphereCount;
    int ConeCount;
    int CapsuleCount;
    int OBBCount;
};

struct FBoundingBoxData
{
    float3 bbMin;
    float padding0;
    float3 bbMax;
    float padding1;
};
struct FConeData
{
    float3 ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름
    
    float3 ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    float4 Color;
    
    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float pad[3];
};
struct FSphereData
{
    float3 Center; // 중심
    float Radius; // 반지름
    float4 Color;
    int SegmentCount; // 세그먼트 수 (위도/경도)
    float Padding[3];
};

struct FCapsuleData
{
    float3 Start;
    float Radius;
    float3 End;
    float Height;
    float4 Color;
    int SegmentCount;
    float3 Padding;
};
struct FOrientedBoxCornerData
{
    float4 corners[8]; // 회전/이동 된 월드 공간상의 8꼭짓점
    float4 Color;
};

StructuredBuffer<FBoundingBoxData> g_BoundingBoxes : register(t2);
StructuredBuffer<FConeData> g_ConeData : register(t3);
StructuredBuffer<FOrientedBoxCornerData> g_OrientedBoxes : register(t4);
StructuredBuffer<FSphereData> SphereBuffer : register(t5);
StructuredBuffer<FCapsuleData> CapsuleBuffer : register(t6);
static const int BB_EdgeIndices[12][2] =
{
    { 0, 1 },
    { 1, 3 },
    { 3, 2 },
    { 2, 0 }, // 앞면
    { 4, 5 },
    { 5, 7 },
    { 7, 6 },
    { 6, 4 }, // 뒷면
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 } // 측면
};

struct VS_INPUT
{
    uint vertexID : SV_VertexID; // 0 또는 1: 각 라인의 시작과 끝
    uint instanceID : SV_InstanceID; // 인스턴스 ID로 grid, axis, bounding box를 구분
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 WorldPosition : POSITION;
    float4 Color : COLOR;
    uint instanceID : SV_InstanceID;
};

/////////////////////////////////////////////////////////////////////////
// Grid 위치 계산 함수
/////////////////////////////////////////////////////////////////////////
float3 ComputeGridPosition(uint instanceID, uint vertexID)
{
    int halfCount = GridCount / 2;
    float centerOffset = halfCount * 0.5; // grid 중심이 원점에 오도록

    float3 startPos;
    float3 endPos;
    
    if (instanceID < halfCount)
    {
        // 수직선: X 좌표 변화, Y는 -centerOffset ~ +centerOffset
        float x = GridOrigin.x + (instanceID - centerOffset) * GridSpacing;
        if (abs(x - GridOrigin.x) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(0, (GridOrigin.y - centerOffset * GridSpacing), 0);
        }
        else
        {
            startPos = float3(x, GridOrigin.y - centerOffset * GridSpacing, GridOrigin.z);
            endPos = float3(x, GridOrigin.y + centerOffset * GridSpacing, GridOrigin.z);
        }
    }
    else
    {
        // 수평선: Y 좌표 변화, X는 -centerOffset ~ +centerOffset
        int idx = instanceID - halfCount;
        float y = GridOrigin.y + (idx - centerOffset) * GridSpacing;
        if (abs(y - GridOrigin.y) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(-(GridOrigin.x + centerOffset * GridSpacing), 0, 0);
        }
        else
        {
            startPos = float3(GridOrigin.x - centerOffset * GridSpacing, y, GridOrigin.z);
            endPos = float3(GridOrigin.x + centerOffset * GridSpacing, y, GridOrigin.z);
        }

    }
    return (vertexID == 0) ? startPos : endPos;
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////
float3 ComputeAxisPosition(uint axisInstanceID, uint vertexID)
{
    float3 start = float3(0.0, 0.0, 0.0);
    float3 end;
    float zOffset = 0.f;
    if (axisInstanceID == 0)
    {
        // X 축: 빨간색
        end = float3(1000000.0, 0.0, zOffset);
    }
    else if (axisInstanceID == 1)
    {
        // Y 축: 초록색
        end = float3(0.0, 1000000.0, zOffset);
    }
    else if (axisInstanceID == 2)
    {
        // Z 축: 파란색
        end = float3(0.0, 0.0, 1000000.0 + zOffset);
    }
    else
    {
        end = start;
    }
    return (vertexID == 0) ? start : end;
}

/////////////////////////////////////////////////////////////////////////
// Bounding Box 위치 계산 함수
// bbInstanceID: StructuredBuffer에서 몇 번째 bounding box인지
// edgeIndex: 해당 bounding box의 12개 엣지 중 어느 것인지
/////////////////////////////////////////////////////////////////////////
float3 ComputeBoundingBoxPosition(uint bbInstanceID, uint edgeIndex, uint vertexID)
{
    FBoundingBoxData box = g_BoundingBoxes[bbInstanceID];
  
//    0: (bbMin.x, bbMin.y, bbMin.z)
//    1: (bbMax.x, bbMin.y, bbMin.z)
//    2: (bbMin.x, bbMax.y, bbMin.z)
//    3: (bbMax.x, bbMax.y, bbMin.z)
//    4: (bbMin.x, bbMin.y, bbMax.z)
//    5: (bbMax.x, bbMin.y, bbMax.z)
//    6: (bbMin.x, bbMax.y, bbMax.z)
//    7: (bbMax.x, bbMax.y, bbMax.z)
    int vertIndex = BB_EdgeIndices[edgeIndex][vertexID];
    float x = ((vertIndex & 1) == 0) ? box.bbMin.x : box.bbMax.x;
    float y = ((vertIndex & 2) == 0) ? box.bbMin.y : box.bbMax.y;
    float z = ((vertIndex & 4) == 0) ? box.bbMin.z : box.bbMax.z;
    return float3(x, y, z);
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////
// Cone 계산 함수
/////////////////////////////////////////////////
// Helper: 계산된 각도에 따른 밑면 꼭짓점 위치 계산

float3 ComputeConePosition(uint globalInstanceID, uint vertexID)
{
    // 모든 cone이 동일한 세그먼트 수를 가짐
    int N = g_ConeData[0].ConeSegmentCount;
    
    uint coneIndex = globalInstanceID / (2 * N);
    uint lineIndex = globalInstanceID % (2 * N);
    
    // cone 데이터 읽기
    FConeData cone = g_ConeData[coneIndex];
    
    // cone의 축 계산
    float3 axis = normalize(cone.ConeApex - cone.ConeBaseCenter);
    
    // axis에 수직인 두 벡터(u, v)를 생성
    float3 arbitrary = abs(dot(axis, float3(0, 0, 1))) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 u = normalize(cross(axis, arbitrary));
    float3 v = cross(axis, u);
    
    if (lineIndex < (uint) N)
    {
        // 측면 선분: cone의 꼭짓점과 밑면의 한 점을 잇는다.
        float angle = lineIndex * 6.28318530718 / N;
        float3 baseVertex = cone.ConeBaseCenter + (cos(angle) * u + sin(angle) * v) * cone.ConeRadius;
        return (vertexID == 0) ? cone.ConeApex : baseVertex;
    }
    else
    {
        // 밑면 둘레 선분: 밑면상의 인접한 두 점을 잇는다.
        uint idx = lineIndex - N;
        float angle0 = idx * 6.28318530718 / N;
        float angle1 = ((idx + 1) % N) * 6.28318530718 / N;
        float3 v0 = cone.ConeBaseCenter + (cos(angle0) * u + sin(angle0) * v) * cone.ConeRadius;
        float3 v1 = cone.ConeBaseCenter + (cos(angle1) * u + sin(angle1) * v) * cone.ConeRadius;
        return (vertexID == 0) ? v0 : v1;
    }
}
/////////////////////////////////////////////////////////////////////////
// OBB
/////////////////////////////////////////////////////////////////////////
float3 ComputeOrientedBoxPosition(uint obIndex, uint edgeIndex, uint vertexID)
{
    FOrientedBoxCornerData ob = g_OrientedBoxes[obIndex];
    int cornerID = BB_EdgeIndices[edgeIndex][vertexID];
    return ob.corners[cornerID].xyz;
}
/////////////////////////////////////////////////////////////////////////
// Sphere
/////////////////////////////////////////////////////////////////////////
float3 ComputeSpherePosition(uint sphereInstanceID, uint vertexID)
{
    int seg = SphereBuffer[0].SegmentCount;
    int ringCount = seg - 1;
    int segCount = seg;
    int totalLines = ringCount * segCount * 2 + segCount * 2;

    int sphereIndex = sphereInstanceID / totalLines;
    int localID = sphereInstanceID % totalLines;
    if (sphereIndex >= SphereCount)
        return float3(0, 0, 0);

    float3 center = SphereBuffer[sphereIndex].Center;
    float radius = SphereBuffer[sphereIndex].Radius;

    if (localID < ringCount * segCount * 2)
    {
        int ring = localID / (segCount * 2);
        int segIdx = (localID / 2) % segCount;
        bool isVertical = (localID % 2 == 0);

        float theta = ring * PI / seg;
        float nextTheta = (ring + 1) * PI / seg;
        float phi = segIdx * 2.0 * PI / seg;

        float3 p0, p1;
        if (isVertical)
        {
            p0 = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
            p1 = float3(sin(nextTheta) * cos(phi), cos(nextTheta), sin(nextTheta) * sin(phi));
        }
        else
        {
            float nextPhi = ((segIdx + 1) % segCount) * 2.0 * PI / seg;
            p0 = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
            p1 = float3(sin(theta) * cos(nextPhi), cos(theta), sin(theta) * sin(nextPhi));
        }

        return (vertexID == 0) ? p0 * radius + center : p1 * radius + center;
    }
    else
    {
        int segIdx = localID - ringCount * segCount * 2;
        bool isTop = segIdx < segCount;
        int idx = segIdx % segCount;
        float phi = idx * 2.0 * PI / seg;
        float theta = isTop ? (PI / seg) : ((seg - 1) * PI / seg);

        float3 pole = float3(0, isTop ? 1 : -1, 0);
        float3 nextDir = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

        return (vertexID == 0) ? center + radius * pole : center + radius * nextDir;
    }
}

/*float3 ComputeSpherePosition(uint sphereInstanceID, uint vertexID)
{
    int seg = SphereBuffer[0].SegmentCount;
    int ringCount = seg - 1;
    int segCount = seg;
    int segmentPerSphere = ringCount * segCount * 2;

    int sphereIndex = sphereInstanceID / segmentPerSphere;
    int localID = sphereInstanceID % segmentPerSphere;
    if (sphereIndex >= SphereCount)
        return float3(0, 0, 0);

    int ring = localID / (segCount * 2);
    int segIdx = (localID / 2) % segCount;
    bool isVertical = (localID % 2 == 0);

    float theta0 = ring * 3.141592 / seg;
    float theta1 = (ring + 1) * 3.141592 / seg;
    float phi = segIdx * 2.0 * 3.141592 / seg;

    float3 p0 = float3(sin(theta0) * cos(phi), cos(theta0), sin(theta0) * sin(phi)) * SphereBuffer[sphereIndex].Radius + SphereBuffer[sphereIndex].Center;
    float3 p1 = float3(sin(theta1) * cos(phi), cos(theta1), sin(theta1) * sin(phi)) * SphereBuffer[sphereIndex].Radius + SphereBuffer[sphereIndex].Center;

    return (vertexID == 0) ? p0 : p1;
}*/

/////////////////////////////////////////////////////////////////////////
// Capsule 계산 함수
/////////////////////////////////////////////////////////////////////////
float3 ComputeCapsulePosition(uint capsuleInstanceID, uint vertexID)
{
    int seg = CapsuleBuffer[0].SegmentCount;
    int totalLinesPerCapsule = seg + seg * seg * 2 + seg; // 원통 + 반구(상하) + 수평 링

    int capsuleIndex = capsuleInstanceID / totalLinesPerCapsule;
    int localID = capsuleInstanceID % totalLinesPerCapsule;

    if (capsuleIndex >= CapsuleCount)
        return float3(0, 0, 0);

    FCapsuleData capsule = CapsuleBuffer[capsuleIndex];
    float3 axis = normalize(capsule.End - capsule.Start);
    float3 arbitrary = abs(dot(axis, float3(0, 0, 1))) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 u = normalize(cross(axis, arbitrary));
    float3 v = cross(axis, u);

    float3 center = 0.5 * (capsule.Start + capsule.End);

    if (localID < seg)
    {
        float angle = localID * (2.0 * PI / seg);
        float3 offset = cos(angle) * u + sin(angle) * v;
        return (vertexID == 0) ? capsule.Start + offset * capsule.Radius : capsule.End + offset * capsule.Radius;
    }
    else if (localID < seg + seg * seg)
    {
        int ring = (localID - seg) / seg;
        int segIdx = (localID - seg) % seg;
        float theta0 = ring * 0.5 * PI / seg;
        float theta1 = (ring + 1) * 0.5 * PI / seg;
        float phi = segIdx * 2.0 * PI / seg;
        float3 dir0 = sin(theta0) * cos(phi) * u + cos(theta0) * (-axis) + sin(theta0) * sin(phi) * v;
        float3 dir1 = sin(theta1) * cos(phi) * u + cos(theta1) * (-axis) + sin(theta1) * sin(phi) * v;
        float3 p0 = capsule.Start + capsule.Radius * dir0;
        float3 p1 = capsule.Start + capsule.Radius * dir1;
        return (vertexID == 0) ? p0 : p1;
    }
    else if (localID < seg + seg * seg * 2)
    {
        int ring = (localID - (seg + seg * seg)) / seg;
        int segIdx = (localID - (seg + seg * seg)) % seg;
        float theta0 = ring * 0.5 * PI / seg;
        float theta1 = (ring + 1) * 0.5 * PI / seg;
        float phi = segIdx * 2.0 * PI / seg;
        float3 dir0 = sin(theta0) * cos(phi) * u + cos(theta0) * axis + sin(theta0) * sin(phi) * v;
        float3 dir1 = sin(theta1) * cos(phi) * u + cos(theta1) * axis + sin(theta1) * sin(phi) * v;
        float3 p0 = capsule.End + capsule.Radius * dir0;
        float3 p1 = capsule.End + capsule.Radius * dir1;
        return (vertexID == 0) ? p0 : p1;
    }
    else
    {
        int ringIdx = localID - (seg + seg * seg * 2);
        float angle = ringIdx * 2.0 * PI / seg;
        float3 offset = cos(angle) * u + sin(angle) * v;
        float3 p0 = center + capsule.Radius * offset;
        float3 p1 = p0 + axis * 0.01;
        return (vertexID == 0) ? p0 : p1;
    }
}


/////////////////////////////////////////////////////////////////////////
// 메인 버텍스 셰이더
/////////////////////////////////////////////////////////////////////////
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    float3 pos;
    float4 color;
    
    // Cone 하나당 (2 * SegmentCount) 선분.
    // ConeCount 개수만큼이므로 총 (2 * SegmentCount * ConeCount).
    uint coneInstCnt = ConeCount * 2 * g_ConeData[0].ConeSegmentCount;

    // Grid / Axis / AABB 인스턴스 개수 계산
    uint gridLineCount = GridCount; // 그리드 라인
    uint axisCount = 3; // X, Y, Z 축 (월드 좌표축)
    uint aabbInstanceCount = 12 * BoundingBoxCount; // AABB 하나당 12개 엣지

    // 1) "콘 인스턴스 시작" 지점
    uint coneInstanceStart = gridLineCount + axisCount + aabbInstanceCount;
    // 2) 그 다음(=콘 구간의 끝)이 곧 OBB 시작 지점
    uint obbStart = coneInstanceStart + coneInstCnt;
    uint sphereStart = obbStart + 12 * OBBCount;
;
    int seg = SphereBuffer[0].SegmentCount;
    int ringCount = seg - 1;
    uint sphereInstCnt = SphereCount * (ringCount * seg * 2 + seg * 2);

    uint capsuleStart = sphereStart + sphereInstCnt;
    seg = CapsuleBuffer[0].SegmentCount;
    uint capsuleInstCnt = CapsuleCount * ((seg - 1) * seg * 2 + seg * 2 + seg);// 이제 instanceID를 기준으로 분기
    if (input.instanceID < gridLineCount)
    {
        // 0 ~ (GridCount-1): 그리드
        pos = ComputeGridPosition(input.instanceID, input.vertexID);
        color = float4(0.1, 0.1, 0.1, 1.0);
    }
    else if (input.instanceID < gridLineCount + axisCount)
    {
        // 그 다음 (axisCount)개: 축(Axis)
        uint axisInstanceID = input.instanceID - gridLineCount;
        pos = ComputeAxisPosition(axisInstanceID, input.vertexID);

        // 축마다 색상
        if (axisInstanceID == 0)
            color = float4(1.0, 0.0, 0.0, 1.0); // X: 빨강
        else if (axisInstanceID == 1)
            color = float4(0.0, 1.0, 0.0, 1.0); // Y: 초록
        else
            color = float4(0.0, 0.0, 1.0, 1.0); // Z: 파랑
    }
    else if (input.instanceID < gridLineCount + axisCount + aabbInstanceCount)
    {
        // 그 다음 AABB 인스턴스 구간
        uint index = input.instanceID - (gridLineCount + axisCount);
        uint bbInstanceID = index / 12; // 12개가 1박스
        uint bbEdgeIndex = index % 12;
        
        pos = ComputeBoundingBoxPosition(bbInstanceID, bbEdgeIndex, input.vertexID);
        color = float4(1.0, 1.0, 0.0, 1.0); // 노란색
    }
    else if (input.instanceID < obbStart)
    {
        // 그 다음 콘(Cone) 구간
        uint coneInstanceID = input.instanceID - coneInstanceStart;
        pos = ComputeConePosition(coneInstanceID, input.vertexID);
        int N = g_ConeData[0].ConeSegmentCount;
        uint coneIndex = coneInstanceID / (2 * N);
        
        color = g_ConeData[coneIndex].Color;
   
        
    }
    else if (input.instanceID < sphereStart)
    {
        uint obbLocalID = input.instanceID - obbStart;
        uint obbIndex = obbLocalID / 12;
        uint edgeIndex = obbLocalID % 12;

        pos = ComputeOrientedBoxPosition(obbIndex, edgeIndex, input.vertexID);
        color = g_OrientedBoxes[obbIndex].Color;
    }
    else if (input.instanceID < capsuleStart)
    {
        uint sphereInstanceID = input.instanceID - sphereStart;
        pos = ComputeSpherePosition(sphereInstanceID, input.vertexID);
        color = SphereBuffer[0].Color; // 또는 sphereInstanceID 인덱싱된 색상
    }
    else
    {
        uint capsuleGlobalInstanceID = input.instanceID - capsuleStart;
        int seg = CapsuleBuffer[0].SegmentCount;
        int linesPerCapsule = seg + seg * seg * 2 + seg;

        uint capsuleIndex = capsuleGlobalInstanceID / linesPerCapsule;
        uint capsuleLocalID = capsuleGlobalInstanceID % linesPerCapsule;

        pos = ComputeCapsulePosition(capsuleGlobalInstanceID, input.vertexID);
        color = CapsuleBuffer[capsuleIndex].Color;
    }


    // 출력 변환
    output.Position = float4(pos, 1.f);
    output.Position = mul(output.Position, WorldMatrix);
    output.WorldPosition = output.Position;
    
    output.Position = mul(output.Position, ViewMatrix);
    output.Position = mul(output.Position, ProjectionMatrix);

    output.Color = color;
    output.instanceID = input.instanceID;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    if (input.instanceID < GridCount || input.instanceID < GridCount + 3)
    {
        float Dist = length(input.WorldPosition.xyz - ViewWorldLocation);

        float MaxDist = 400 * 1.2f;
        float MinDist = MaxDist * 0.3f;

         // Fade out grid
        float Fade = saturate(1.f - (Dist - MinDist) / (MaxDist - MinDist));
        input.Color.a *= Fade * Fade * Fade;
    }
    return input.Color;
}
