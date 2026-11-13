#include "LunarGo2Pawn.h"
#include "Camera/CameraComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "mujoco/mujoco.h"
#include "onnxruntime_cxx_api.h"
#include <array>
#include <memory>

// Observation/action layout follows the MIT/BSD go2_rl_gym deployment interface.
struct FGo2Runtime {
    mjModel* Model = nullptr;
    mjData* Data = nullptr;
    Ort::Env Environment{ORT_LOGGING_LEVEL_WARNING, "LunarGo2"};
    std::unique_ptr<Ort::Session> Policy;
    std::array<float,225> History{};
    std::array<float,12> PreviousAction{};
    std::array<float,12> Target{};
    const std::array<float,12> Home{.1f,.8f,-1.5f,-.1f,.8f,-1.5f,.1f,1.f,-1.5f,-.1f,1.f,-1.5f};
    int StepIndex = 0;
    ~FGo2Runtime() { if (Data) mj_deleteData(Data); if (Model) mj_deleteModel(Model); }
    void Step(float Forward, float Turn) {
        if (StepIndex++ % 10 == 0) {
            float Obs[45]{};
            const mjtNum* Q=Data->qpos;
            const mjtNum* V=Data->qvel;
            for (int I=0; I<3; ++I) Obs[I]=V[I+3]*.25f;
            mjtNum Rotation[9], Gravity[3]{0,0,-1}, LocalGravity[3];
            mju_quat2Mat(Rotation,Q+3);
            mju_mulMatTVec(LocalGravity,Rotation,Gravity,3,3);
            for (int I=0; I<3; ++I) Obs[I+3]=LocalGravity[I];
            Obs[6]=2.f*Forward; Obs[8]=.25f*Turn;
            for (int I=0; I<12; ++I) {
                Obs[9+I]=Q[7+I]-Home[I]; Obs[21+I]=V[6+I]*.05f; Obs[33+I]=PreviousAction[I];
            }
            auto Memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
            int64_t ObsShape[]{1,45}, HistoryShape[]{1,5,45};
            std::array<Ort::Value,2> Inputs{
                Ort::Value::CreateTensor<float>(Memory,Obs,45,ObsShape,2),
                Ort::Value::CreateTensor<float>(Memory,History.data(),225,HistoryShape,3)};
            const char* InputNames[]{"obs","hist"}; const char* OutputNames[]{"act","new_hist"};
            auto Outputs=Policy->Run(Ort::RunOptions{nullptr},InputNames,Inputs.data(),2,OutputNames,2);
            const float* Action=Outputs[0].GetTensorData<float>();
            for (int I=0; I<12; ++I) { PreviousAction[I]=Action[I]; Target[I]=Home[I]+.25f*Action[I]; }
            FMemory::Memcpy(History.data(),Outputs[1].GetTensorData<float>(),sizeof(History));
        }
        for (int I=0; I<12; ++I) Data->ctrl[I]=20*(Target[I]-Data->qpos[7+I])-.5*Data->qvel[6+I];
        mj_step(Model,Data);
    }
};

ALunarGo2Pawn::ALunarGo2Pawn() {
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("RearCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->FieldOfView=90;
}
ALunarGo2Pawn::~ALunarGo2Pawn() = default;
void ALunarGo2Pawn::BeginPlay() {
    Super::BeginPlay();
    try {
        Runtime=MakeShared<FGo2Runtime>();
        const FString Content=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
        const FString Xml=Content/TEXT("model/go2/scene.xml");
        char Error[2048]{};
        Runtime->Model=mj_loadXML(TCHAR_TO_UTF8(*Xml),nullptr,Error,sizeof(Error));
        if (!Runtime->Model) { UE_LOG(LogTemp,Error,TEXT("Go2 XML: %s"),UTF8_TO_TCHAR(Error)); Runtime.Reset(); return; }
        mjModel* M=Runtime->Model;
        TArray<uint8> Height;
        const int64 Count=int64(M->hfield_nrow[0])*M->hfield_ncol[0];
        if (!FFileHelper::LoadFileToArray(Height,*(Content/TEXT("RuntimeTerrain/terrain_hfield.bin"))) || Height.Num()!=Count*sizeof(float)) {
            UE_LOG(LogTemp,Error,TEXT("Missing or incompatible collision heightfield")); Runtime.Reset(); return;
        }
        FMemory::Memcpy(M->hfield_data+M->hfield_adr[0],Height.GetData(),Height.Num());
        M->opt.gravity[2]=-9.81;
        Runtime->Data=mj_makeData(M);
        Ort::SessionOptions Options; Options.SetIntraOpNumThreads(1);
        const FString Onnx=Content/TEXT("model/go2/go2_moe.onnx");
        Runtime->Policy=std::make_unique<Ort::Session>(Runtime->Environment,TCHAR_TO_UTF8(*Onnx),Options);
        ResetRobot(); BuildRobot(); UpdateVisuals();
        bDemo=FParse::Param(FCommandLine::Get(),TEXT("Demo"));
        if (APlayerController* PC=Cast<APlayerController>(GetController())) {
            PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false;
        }
        GEngine->Exec(GetWorld(),TEXT("wp.Runtime.OverrideRuntimeLoadingRange -grid=MainGrid -range=100000"));
        UE_LOG(LogTemp,Display,TEXT("LUNARGO2_READY gravity=9.81 speed=1 camera=rear"));
    } catch (const std::exception& E) { UE_LOG(LogTemp,Error,TEXT("Go2 initialization: %s"),UTF8_TO_TCHAR(E.what())); Runtime.Reset(); }
}
void ALunarGo2Pawn::EndPlay(const EEndPlayReason::Type Reason) { Runtime.Reset(); Super::EndPlay(Reason); }
void ALunarGo2Pawn::ResetRobot() {
    mj_resetDataKeyframe(Runtime->Model,Runtime->Data,0);
    Runtime->History.fill(0); Runtime->PreviousAction.fill(0); Runtime->Target=Runtime->Home;
    Runtime->StepIndex=0; Accumulator=0; CameraOrbit=0;
    mj_forward(Runtime->Model,Runtime->Data);
}
void ALunarGo2Pawn::BuildRobot() {
    const mjModel* M=Runtime->Model;
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MuJoCo/RobotMat.RobotMat"));
    for (int G=0; G<M->ngeom; ++G) {
        if (M->geom_type[G]!=mjGEOM_MESH || M->geom_group[G]!=2) continue;
        const int Mesh=M->geom_dataid[G], VertexStart=M->mesh_vertadr[Mesh], FaceStart=M->mesh_faceadr[Mesh];
        TArray<FVector> Vertices, Normals; TArray<int32> Triangles; TArray<FVector2D> UV;
        for (int F=0; F<M->mesh_facenum[Mesh]; ++F) {
            for (int K : {0,2,1}) {
                const int Corner=3*(FaceStart+F)+K;
                const int V=3*(VertexStart+M->mesh_face[Corner]);
                Vertices.Emplace(M->mesh_vert[V]*100,M->mesh_vert[V+1]*100,M->mesh_vert[V+2]*100);
                int N=3*(M->mesh_normaladr[Mesh]+M->mesh_facenormal[Corner]);
                Normals.Emplace(M->mesh_normal[N],M->mesh_normal[N+1],M->mesh_normal[N+2]);
                const int Tex=M->mesh_facetexcoord[Corner];
                int T=2*(M->mesh_texcoordadr[Mesh]+Tex);
                UV.Add(Tex>=0 ? FVector2D(M->mesh_texcoord[T],M->mesh_texcoord[T+1]) : FVector2D::ZeroVector);
                Triangles.Add(Triangles.Num());
            }
        }
        TArray<FProcMeshTangent> Tangents; TArray<FVector> ComputedNormals;
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices,Triangles,UV,ComputedNormals,Tangents);
        auto* Component=NewObject<UProceduralMeshComponent>(this);
        Component->SetupAttachment(RootComponent); Component->RegisterComponent();
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV,TArray<FLinearColor>(),Tangents,false);
        auto* Instance=UMaterialInstanceDynamic::Create(Material,this);
        const FString Name=UTF8_TO_TCHAR(mj_id2name(M,mjOBJ_MESH,Mesh));
        FLinearColor Color(.46f,.48f,.51f,1);
        if (Name==TEXT("base_1")) Color=FLinearColor(.7,.7,.7,1);
        if (Name==TEXT("base_2") || Name==TEXT("base_3") || Name.Contains(TEXT("calf_1")) || Name==TEXT("foot")) Color=FLinearColor(.04,.04,.045,1);
        Instance->SetVectorParameterValue(TEXT("BaseColor"),Color*.65f);
        Instance->SetScalarParameterValue(TEXT("Roughness"),.85f);
        Instance->SetScalarParameterValue(TEXT("Metallic"),0);
        Instance->SetScalarParameterValue(TEXT("Fill"),0);
        Component->SetMaterial(0,Instance);
        Visuals.Add(Component); GeometryIds.Add(G);
    }
}
void ALunarGo2Pawn::UpdateVisuals() {
    const mjData* D=Runtime->Data;
    const FVector Base(D->xpos[3]*100,D->xpos[4]*100,D->xpos[5]*100);
    SetActorLocation(Base);
    for (int I=0; I<Visuals.Num(); ++I) {
        const int G=GeometryIds[I]; mjtNum Q[4]; mju_mat2Quat(Q,D->geom_xmat+G*9);
        Visuals[I]->SetWorldLocationAndRotation(FVector(D->geom_xpos[G*3]*100,D->geom_xpos[G*3+1]*100,D->geom_xpos[G*3+2]*100),FQuat(Q[1],Q[2],Q[3],Q[0]));
    }
    const mjtNum* Q=D->xquat+4;
    const double Yaw=FQuat(Q[1],Q[2],Q[3],Q[0]).Rotator().Yaw+CameraOrbit;
    const double Angle=FMath::DegreesToRadians(Yaw);
    Camera->SetWorldLocation(Base+FVector(-CameraDistance*FMath::Cos(Angle),-CameraDistance*FMath::Sin(Angle),40));
    Camera->SetWorldRotation(FRotator(CameraPitch,Yaw,0));
}
void ALunarGo2Pawn::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    if (!Runtime) return;
    APlayerController* PC=Cast<APlayerController>(GetController());
    float Forward=0,Turn=0;
    if (PC) {
        Forward=float(PC->IsInputKeyDown(EKeys::W))-float(PC->IsInputKeyDown(EKeys::S));
        Turn=.45f*(float(PC->IsInputKeyDown(EKeys::D))-float(PC->IsInputKeyDown(EKeys::A)));
        if (PC->WasInputKeyJustPressed(EKeys::R)) ResetRobot();
        if (PC->WasInputKeyJustPressed(EKeys::Escape)) PC->ConsoleCommand(TEXT("quit"));
        if (PC->IsInputKeyDown(EKeys::RightMouseButton)) {
            float X,Y;PC->GetInputMouseDelta(X,Y); CameraOrbit+=X; CameraPitch=FMath::Clamp(CameraPitch-Y,-60.f,35.f);
        }
        CameraDistance=FMath::Clamp(CameraDistance-30*PC->GetInputAnalogKeyState(EKeys::MouseWheelAxis),80.f,600.f);
    }
    if (bDemo) {
        double T=Runtime->Data->time;
        Forward=(T>2 && T<10) ? .6f : 0;
        Turn=(T>6 && T<9) ? .15f : 0;
    }
    Accumulator+=DeltaTime;
    // Consume real elapsed time with fixed physics substeps, never accelerated game time.
    const double Step=Runtime->Model->opt.timestep;
    try {
        while (Accumulator>=Step) { Runtime->Step(Forward,Turn); Accumulator-=Step; }
    } catch (const std::exception& E) { UE_LOG(LogTemp,Error,TEXT("Policy inference: %s"),UTF8_TO_TCHAR(E.what())); Runtime.Reset(); return; }
    UpdateVisuals();
    if (bDemo && Runtime->Data->time-LastDiagnostic>1) {
        LastDiagnostic=Runtime->Data->time;
        const mjData* D=Runtime->Data;
        UE_LOG(LogTemp,Display,TEXT("GO2_STATE t=%.2f x=%.3f y=%.3f z=%.3f upright=%.3f speed=%.3f"),D->time,D->qpos[0],D->qpos[1],D->qpos[2],D->xmat[9+8],FMath::Sqrt(D->qvel[0]*D->qvel[0]+D->qvel[1]*D->qvel[1]));
        if (D->time>14) GEngine->Exec(GetWorld(),TEXT("quit"));
    }
}
