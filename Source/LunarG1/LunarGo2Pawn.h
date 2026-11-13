#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LunarGo2Pawn.generated.h"
class UCameraComponent;
class UProceduralMeshComponent;
struct FGo2Runtime;

UCLASS()
class LUNARG1_API ALunarGo2Pawn : public APawn {
    GENERATED_BODY()
public:
    ALunarGo2Pawn();
    virtual ~ALunarGo2Pawn();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaTime) override;
private:
    void ResetRobot();
    void BuildRobot();
    void UpdateVisuals();
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Visuals;
    TArray<int32> GeometryIds;
    TSharedPtr<FGo2Runtime> Runtime;
    double Accumulator = 0;
    float CameraDistance = 120;
    float CameraPitch = -12;
    float CameraOrbit = 0;
    bool bDemo = false;
    double LastDiagnostic = -1;
};
