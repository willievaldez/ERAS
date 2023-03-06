#include "Portal.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMaterialLibrary.h"

#include "ERASCharacter.h"

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostPhysics;

	RootComponent = SphereRoot = CreateDefaultSubobject<USphereComponent>(USphereComponent::GetDefaultSceneRootVariableName());
	SphereRoot->SetCollisionProfileName("OverlapAll");

	//DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	//DoorFrame->SetupAttachment(RootComponent);

	PrevPortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrevPortalMesh"));
	PrevPortalMesh->SetCollisionProfileName("OverlapAll");
	PrevPortalMesh->bHiddenInSceneCapture = true;
	PrevPortalMesh->CastShadow = false;
	PrevPortalMesh->SetupAttachment(RootComponent);

	NextPortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NextPortalMesh"));
	NextPortalMesh->SetCollisionProfileName("OverlapAll");
	NextPortalMesh->bHiddenInSceneCapture = true;
	NextPortalMesh->CastShadow = false;
	NextPortalMesh->SetupAttachment(RootComponent);

	View = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	View->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	View->bEnableClipPlane = true;
	View->bUseCustomProjectionMatrix = true;
	View->bCaptureEveryFrame = false;
	View->bCaptureOnMovement = false;
	View->bAlwaysPersistRenderingState = true;

	// Quality Flags
	//View->PostProcessSettings.bOverride_AmbientOcclusionQuality = true;
	//View->PostProcessSettings.AmbientOcclusionQuality = 0.0f;
	//View->PostProcessSettings.bOverride_MotionBlurAmount = true;
	//View->PostProcessSettings.MotionBlurAmount = 0.0f;
	//View->PostProcessSettings.bOverride_SceneFringeIntensity = true;
	//View->PostProcessSettings.SceneFringeIntensity = 0.0f;
	//View->PostProcessSettings.bOverride_FilmGrainIntensity = true;
	//View->PostProcessSettings.FilmGrainIntensity = 0.0f;
	//View->PostProcessSettings.bOverride_ScreenSpaceReflectionQuality = true;
	//View->PostProcessSettings.ScreenSpaceReflectionQuality = 0.0f;
	//View->ShowFlags.DynamicShadows = false;

	View->SetupAttachment(RootComponent);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();



	PrevMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, MaterialInterface);
	PrevPortalMesh->SetMaterial(0, PrevMaterial);
	NextMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, MaterialInterface);
	NextPortalMesh->SetMaterial(0, NextMaterial);

	//PrevPortalMesh->OnComponentBeginOverlap.AddDynamic(this, &APortal::PortalBeginOverlap);
	//PrevPortalMesh->OnComponentBeginOverlap.AddDynamic(this, &APortal::PortalEndOverlap);
	//SphereRoot->OnComponentBeginOverlap.AddDynamic(this, &APortal::SphereBeginOverlap);
	//SphereRoot->OnComponentEndOverlap.AddDynamic(this, &APortal::SphereEndOverlap);
	NextPortalMesh->OnComponentBeginOverlap.AddDynamic(this, &APortal::PortalBeginOverlap);
	NextPortalMesh->OnComponentEndOverlap.AddDynamic(this, &APortal::PortalEndOverlap);

	IsClient = GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->GetLocalPlayer() != nullptr;

	if (PrevPortal)
	{
		PrevPortal->NotifyOnPortalTextureReady(this, &ThisClass::AttachPrevPortalTexture);
	}

	if (NextPortal)
	{
		NextPortal->NotifyOnPortalTextureReady(this, &ThisClass::AttachNextPortalTexture);
	}

	if (IsClient)
	{
		GeneratePortalTexture();
	}

}

void APortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const TArray<FOverlapInfo>& FOverlapInfo = SphereRoot->GetOverlapInfos();
	if (FOverlapInfo.IsEmpty())
	{
		return;
	}

	if (IsClient)
	{
		APlayerCameraManager* PCM = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;

		FSceneViewProjectionData PlayerProjectionData;
		GetWorld()->GetFirstPlayerController()->GetLocalPlayer()->GetProjectionData(
			GetWorld()->GetFirstPlayerController()->GetLocalPlayer()->ViewportClient->Viewport,
			PlayerProjectionData);

		float DotProduct = GetActorRotation().Vector().Dot(PCM->GetCameraRotation().Vector());
		if (DotProduct < 0 && PrevPortal)
		{
			//PrevOffset = PrevPortal->GetActorLocation() - GetActorLocation();
			PrevPortal->View->SetWorldLocation(PCM->GetCameraLocation() + PrevPortal->GetActorLocation() - GetActorLocation());
			PrevPortal->View->SetWorldRotation(PCM->GetCameraRotation());
			PrevPortal->View->ClipPlaneBase = GetActorLocation() + GetActorRotation().Vector() * 200.0f;
			PrevPortal->View->ClipPlaneNormal = PrevPortal->GetActorRotation().Vector() * -1.f;
			PrevPortal->View->CustomProjectionMatrix = PlayerProjectionData.ProjectionMatrix;
			PrevPortal->View->CaptureScene();
		}

		if (DotProduct > 0 && NextPortal)
		{
			//NextOffset = NextPortal->GetActorLocation() - GetActorLocation();
			NextPortal->View->SetWorldLocation(PCM->GetCameraLocation() + NextPortal->GetActorLocation() - GetActorLocation());
			NextPortal->View->SetWorldRotation(PCM->GetCameraRotation());
			NextPortal->View->ClipPlaneBase = GetActorLocation() - GetActorRotation().Vector() * 200.0f;
			NextPortal->View->ClipPlaneNormal = NextPortal->GetActorRotation().Vector();
			NextPortal->View->CustomProjectionMatrix = PlayerProjectionData.ProjectionMatrix;
			NextPortal->View->CaptureScene();
		}
	}
}

void APortal::SetVisibleTemp(bool Visible)
{
	if (Visible)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: UNHIDING PORTAL MESHES"),
			*GetActorNameOrLabel());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: HIDING PORTAL MESHES"),
			*GetActorNameOrLabel());
	}

	PrevPortalMesh->SetVisibility(Visible);
	NextPortalMesh->SetVisibility(Visible);
}

//void APortal::SphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
//{
//	UE_LOG(LogTemp, Warning, TEXT("%s: BEGIN SPHERE OVERLAP WITH %s"),
//		*GetActorNameOrLabel(), *OtherActor->GetActorNameOrLabel());
//	ActorsInSphere.Add(OtherActor);
//}
//
//void APortal::SphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
//{
//	UE_LOG(LogTemp, Warning, TEXT("%s: END SPHERE OVERLAP WITH %s"),
//		*GetActorNameOrLabel(), *OtherActor->GetActorNameOrLabel());
//	ActorsInSphere.Remove(OtherActor);
//}


void APortal::PortalBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("%s: BEGIN OVERLAP WITH %s"),
		*GetActorNameOrLabel(), *OtherActor->GetActorNameOrLabel());

	AERASCharacter* Character = Cast<AERASCharacter>(OtherActor);

	if (!TeleportEnabled)
	{
		if (IsClient
			&& Character && Character->IsLocallyControlled())
		{
			SetVisibleTemp(false);
		}
		return;
	}

	TeleportEnabled = false;
	float DotProduct = GetActorRotation().Vector().Dot(OtherActor->GetVelocity());
	if (DotProduct < 0 && PrevPortal && PrevPortal->TeleportEnabled)
	{
		SendTeleport(PrevPortal, OtherActor);
	}
	if (DotProduct > 0 && NextPortal && NextPortal->TeleportEnabled)
	{
		SendTeleport(NextPortal, OtherActor);
	}
}

void APortal::PortalEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("%s: END OVERLAP WITH %s"),
		*GetActorNameOrLabel(), *OtherActor->GetActorNameOrLabel());

	AERASCharacter* Character = Cast<AERASCharacter>(OtherActor);

	TeleportEnabled = true;

	if (Character && Character->IsLocallyControlled())
	{
		SetVisibleTemp(true);
	}
}

void APortal::SendTeleport(APortal* DestPortal, AActor* Actor)
{
	TeleportEnabled = false;

	AERASCharacter* Character = Cast<AERASCharacter>(Actor);

	DestPortal->ReceiveTeleport(this, Actor);
}

void APortal::ReceiveTeleport(APortal* SrcPortal, AActor* Actor)
{
	TeleportEnabled = false;

	AERASCharacter* Character = Cast<AERASCharacter>(Actor);

	if (IsClient && Character && Character->IsLocallyControlled())
	{
		APlayerCameraManager* PCM = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
		PCM->bGameCameraCutThisFrame = true;
		SetVisibleTemp(false);
	}
	else
	{
		Actor->SetActorLocation(Actor->GetActorLocation() + GetActorLocation() - SrcPortal->GetActorLocation());
	}

	TeleportEnabled = true;
}

void APortal::AttachPrevPortalTexture()
{
	PrevMaterial->SetTextureParameterValue(FName("Portal"), PrevPortal->PortalViewTextureTarget);
}

void APortal::AttachNextPortalTexture()
{
	NextMaterial->SetTextureParameterValue(FName("Portal"), NextPortal->PortalViewTextureTarget);
}

void APortal::NotifyOnPortalTextureReady(APortal* Listener, void (APortal::* InFunc)())
{
	if (PortalViewTextureTarget)
	{
		(Listener->*InFunc)();
	}
	else
	{
		OnPortalTextureReady.AddUObject(Listener, InFunc);
	}
}

void APortal::GeneratePortalTexture()
{
	if (!PortalViewTextureTarget)
	{
		PortalViewTextureTarget = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), TEXT("PortalViewRenderTarget"));
		PortalViewTextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		PortalViewTextureTarget->Filter = TextureFilter::TF_Bilinear;
		PortalViewTextureTarget->SizeX = GSystemResolution.ResX;
		PortalViewTextureTarget->SizeY = GSystemResolution.ResY;
		PortalViewTextureTarget->ClearColor = FLinearColor::Black;
		PortalViewTextureTarget->TargetGamma = 2.2f;
		PortalViewTextureTarget->bNeedsTwoCopies = false;
		PortalViewTextureTarget->AddressX = TextureAddress::TA_Clamp;
		PortalViewTextureTarget->AddressY = TextureAddress::TA_Clamp;
		PortalViewTextureTarget->bAutoGenerateMips = false;
		View->TextureTarget = PortalViewTextureTarget;

		PortalViewTextureTarget->UpdateResource();
	}

	OnPortalTextureReady.Broadcast();
}
