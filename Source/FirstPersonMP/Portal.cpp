#include "Portal.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"

#include "FirstPersonMPCharacter.h"

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostPhysics;

	Root = RootComponent = CreateDefaultSubobject<USceneComponent>(USceneComponent::GetDefaultSceneRootVariableName());

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	DoorFrame->SetupAttachment(RootComponent);

	PrevPortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrevPortalMesh"));
	PrevPortalMesh->SetCollisionProfileName("OverlapAll");
	PrevPortalMesh->SetupAttachment(RootComponent);

	NextPortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NextPortalMesh"));
	NextPortalMesh->SetCollisionProfileName("OverlapAll");
	NextPortalMesh->SetupAttachment(RootComponent);

	View = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	View->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDRNoAlpha;
	View->SetupAttachment(RootComponent);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();

	View->bEnableClipPlane = true;
	View->ClipPlaneBase = GetActorLocation();
	View->bUseCustomProjectionMatrix = true;

	View->PostProcessSettings.bOverride_AmbientOcclusionQuality = true;
	View->PostProcessSettings.AmbientOcclusionQuality = 0.0f;

	View->PostProcessSettings.bOverride_MotionBlurAmount = true;
	View->PostProcessSettings.MotionBlurAmount = 0.0f;

	View->PostProcessSettings.bOverride_SceneFringeIntensity = true;
	View->PostProcessSettings.SceneFringeIntensity = 0.0f;

	View->PostProcessSettings.bOverride_FilmGrainIntensity = true;
	View->PostProcessSettings.FilmGrainIntensity = 0.0f;

	View->PostProcessSettings.bOverride_ScreenSpaceReflectionQuality = true;
	View->PostProcessSettings.ScreenSpaceReflectionQuality = 0.0f;

	IsClient = GetWorld()->GetFirstPlayerController()->GetLocalPlayer() != nullptr;

	if (IsClient)
	{
		GeneratePortalTexture();
	}

	if (PrevPortal)
	{
		PrevOffset = PrevPortal->GetActorLocation() - GetActorLocation();
		PrevPortal->NotifyOnPortalTextureReady(this, &ThisClass::AttachPrevPortalTexture);
	}

	if (NextPortal)
	{
		NextOffset = NextPortal->GetActorLocation() - GetActorLocation();
		NextPortal->NotifyOnPortalTextureReady(this, &ThisClass::AttachNextPortalTexture);
	}
}

void APortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bShouldUpdateCameraViews && IsClient)
	{
		APlayerCameraManager* PCM = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;

		FSceneViewProjectionData PlayerProjectionData;
		GetWorld()->GetFirstPlayerController()->GetLocalPlayer()->GetProjectionData(
			GetWorld()->GetFirstPlayerController()->GetLocalPlayer()->ViewportClient->Viewport,
			PlayerProjectionData);

		float DotProduct = GetActorRotation().Vector().Dot(PCM->GetCameraRotation().Vector());
		if (DotProduct < 0 && PrevPortal)
		{
			PrevPortal->View->SetWorldLocation(PCM->GetCameraLocation() + PrevOffset);
			PrevPortal->View->SetWorldRotation(PCM->GetCameraRotation());
			PrevPortal->View->ClipPlaneNormal = PrevPortal->GetActorRotation().Vector() * -1.f;
			PrevPortal->View->CustomProjectionMatrix = PlayerProjectionData.ProjectionMatrix;
		}

		if (DotProduct > 0 && NextPortal)
		{
			NextPortal->View->SetWorldLocation(PCM->GetCameraLocation() + NextOffset);
			NextPortal->View->SetWorldRotation(PCM->GetCameraRotation());
			NextPortal->View->ClipPlaneNormal = NextPortal->GetActorRotation().Vector();
			NextPortal->View->CustomProjectionMatrix = PlayerProjectionData.ProjectionMatrix;
		}
	}
}

void APortal::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	UE_LOG(LogTemp, Warning, TEXT("%s: BEGIN OVERLAP WITH %s"),
		*GetName(), *OtherActor->GetName());

	if (!TeleportEnabled)
	{
		return;
	}

	bool bPlayerTeleported = Cast<AFirstPersonMPCharacter>(OtherActor) != nullptr;

	TeleportEnabled = false;
	float DotProduct = GetActorRotation().Vector().Dot(OtherActor->GetVelocity());
	if (DotProduct < 0 && PrevPortal && PrevPortal->TeleportEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Teleporting to Prev: %s"),
			*GetName(), *PrevPortal->GetName());
		PrevPortal->TeleportEnabled = false;

		if (bPlayerTeleported)
		{
			PrevPortal->PrevPortalMesh->SetHiddenInGame(true, true);
			PrevPortal->NextPortalMesh->SetHiddenInGame(true, true);
			PrevPortal->bShouldUpdateCameraViews = true;
			bShouldUpdateCameraViews = false;
		}

		OtherActor->SetActorLocation(OtherActor->GetActorLocation() + PrevOffset);
	}
	if (DotProduct > 0 && NextPortal && NextPortal->TeleportEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Teleporting to Next: %s"),
			*GetName(), *NextPortal->GetName());
		NextPortal->TeleportEnabled = false;
		if (bPlayerTeleported)
		{
			NextPortal->PrevPortalMesh->SetHiddenInGame(true, true);
			NextPortal->NextPortalMesh->SetHiddenInGame(true, true);
			NextPortal->bShouldUpdateCameraViews = true;
			bShouldUpdateCameraViews = false;
		}
		OtherActor->SetActorLocation(OtherActor->GetActorLocation() + NextOffset);
	}
}

void APortal::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	UE_LOG(LogTemp, Warning, TEXT("%s: END OVERLAP WITH %s"),
		*GetName(), *OtherActor->GetName());

	TeleportEnabled = true;
	PrevPortalMesh->SetHiddenInGame(false, true);
	NextPortalMesh->SetHiddenInGame(false, true);
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
