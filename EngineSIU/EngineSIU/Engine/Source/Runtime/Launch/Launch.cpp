#include "Core/HAL/PlatformType.h"
#include "EngineLoop.h"

FEngineLoop GEngineLoop;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // 사용 안하는 파라미터들
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    GEngineLoop.Init(hInstance);
    GEngineLoop.Tick();
    GEngineLoop.Exit();

    return 0;
}

// #include <PxPhysicsAPI.h>
// #include <d3d11.h>
// #include <DirectXMath.h>
// #include <vector>
//
// using namespace physx;
// using namespace DirectX;
//
// // PhysX 전역
// PxDefaultAllocator      gAllocator;
// PxDefaultErrorCallback  gErrorCallback;
// PxFoundation*           gFoundation = nullptr;
// PxPhysics*              gPhysics = nullptr;
// PxScene*                gScene = nullptr;
// PxMaterial*             gMaterial = nullptr;
// PxDefaultCpuDispatcher* gDispatcher = nullptr;
//
// // 게임 오브젝트
// struct GameObject {
//     PxRigidDynamic* rigidBody = nullptr;
//     XMMATRIX worldMatrix = XMMatrixIdentity();
//
//     void UpdateFromPhysics() {
//         PxTransform t = rigidBody->getGlobalPose();
//         PxMat44 mat(t);
//         worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&mat));
//     }
// };
//
// std::vector<GameObject> gObjects;
//
// void InitPhysX() {
//     gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
//     gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale());
//     gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
//
//     PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
//     sceneDesc.gravity = PxVec3(0, -9.81f, 0);
//     gDispatcher = PxDefaultCpuDispatcherCreate(2);
//     sceneDesc.cpuDispatcher = gDispatcher;
//     sceneDesc.filterShader = PxDefaultSimulationFilterShader;
//     gScene = gPhysics->createScene(sceneDesc);
// }
//
// GameObject CreateBox(const PxVec3& pos, const PxVec3& halfExtents) {
//     GameObject obj;
//     PxTransform pose(pos);
//     obj.rigidBody = gPhysics->createRigidDynamic(pose);
//     PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
//     obj.rigidBody->attachShape(*shape);
//     PxRigidBodyExt::updateMassAndInertia(*obj.rigidBody, 10.0f);
//     gScene->addActor(*obj.rigidBody);
//     obj.UpdateFromPhysics();
//     return obj;
// }
//
// void Simulate(float dt) {
//     gScene->simulate(dt);
//     gScene->fetchResults(true);
//     for (auto& obj : gObjects) obj.UpdateFromPhysics();
// }
//
// // 렌더링은 생략 – worldMatrix를 사용해 D3D11에서 월드 행렬로 렌더링
//
// int main() {
//     InitPhysX();
//
//     // 박스 생성
//     gObjects.push_back(CreateBox(PxVec3(0, 5, 0), PxVec3(1,1,1)));
//
//     // 메인 루프 예시
//     while (true) {
//         Simulate(1.0f / 60.0f);
//         // Render(gObjects[i].worldMatrix); // ← 너의 렌더링 코드에 맞춰 사용
//     }
//
//     return 0;
// }
