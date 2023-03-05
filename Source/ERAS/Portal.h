#pragma once

#include "CoreMinimal.h"

#include "Portal.generated.h"

UCLASS(Blueprintable, BlueprintType)
class APortal : public AActor
{
	GENERATED_BODY()

public:
	APortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool TeleportEnabled = true;

	DECLARE_EVENT(APortal, FOnPortalTextureReady);
	FOnPortalTextureReady OnPortalTextureReady;

	void SetVisibleTemp(bool Visible);

	void NotifyOnPortalTextureReady(APortal* Listener, void (APortal::* InFunc)());
	
protected:
	void SendTeleport(APortal* DestPortal, AActor* Actor);
	void ReceiveTeleport(APortal* SrcPortal, AActor* Actor);

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<APortal> PrevPortal;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<APortal> NextPortal;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class USphereComponent> SphereRoot;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> DoorFrame;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> PrevPortalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> NextPortalMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class USceneCaptureComponent2D> View;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite)
	TObjectPtr<class UTextureRenderTarget2D> PortalViewTextureTarget;

	//virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	//virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	UFUNCTION()
	void SphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void SphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void PortalBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void PortalEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void GeneratePortalTexture();

	UFUNCTION(BlueprintImplementableEvent)
	void AttachNextPortalTexture();
	UFUNCTION(BlueprintImplementableEvent)
	void AttachPrevPortalTexture();

	//FVector PrevOffset;
	//FVector NextOffset;
	bool IsClient = false;
	TSet<TObjectPtr<AActor>> ActorsInSphere;
};