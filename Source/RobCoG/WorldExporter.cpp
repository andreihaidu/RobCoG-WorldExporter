// Copyright 2017, Institute for Artificial Intelligence - University of Bremen

#include "WorldExporter.h"
#include "Tags.h"
#include "HAL/PlatformFilemanager.h"
#include "Paths.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"


// Sets default values
AWorldExporter::AWorldExporter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AWorldExporter::BeginPlay()
{
	Super::BeginPlay();

	TMap<UObject*, FString> ObjToClass = FTags::GetObjectsToKeyValue(GetWorld(), TEXT("SemLog"), TEXT("Class"));
	TMap<UObject*, FString> ObjToId = FTags::GetObjectsToKeyValue(GetWorld(), TEXT("SemLog"), TEXT("Id"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString ExportDir = FPaths::GameDir() + TEXT("/WorldExport/");
	if (PlatformFile.CreateDirectoryTree(*ExportDir))
	{
		FString Indent = TEXT("");
		FString Text = TEXT("---\n");
		Text.Append(Indent + TEXT("[\n"));
		Indent = Indent.Append(TEXT("  "));

		for (const auto& ObjToClassItr : ObjToClass)
		{
			UObject* CurrObj = ObjToClassItr.Key;
			Text.Append(Indent + "{\n");
			Indent = Indent.Append(TEXT("  "));
			Text.Append(Indent + TEXT("Class:" + ObjToClassItr.Value + TEXT(",\n")));
			if (ObjToId.Contains(CurrObj))
			{
				Text.Append(Indent + TEXT("Id:" + ObjToId[CurrObj] + TEXT(",\n")));
			}
			Text.Append(Indent + TEXT("Mesh:" + ObjToClassItr.Value + TEXT(".dae,\n")));


			if (AStaticMeshActor* ObjAsActor = Cast<AStaticMeshActor>(CurrObj))
			{
				FVector Loc = ObjAsActor->GetActorLocation() * 0.01f; // ToMeters
				FQuat Quat = ObjAsActor->GetActorQuat();
				FString Pose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f]"),Loc.X, Loc.Y, Loc.Z, Quat.W, Quat.X, Quat.Y, Quat.Z);
				Text.Append(Indent + TEXT("Pose: " + Pose + ",\n"));
				
				if (UStaticMeshComponent* SMComp = ObjAsActor->GetStaticMeshComponent())
				{
					Text.Append(Indent + FString::Printf(TEXT("Mass: %f,\n"), SMComp->GetMass()));
				}

				if (AActor* AttachedParent = ObjAsActor->GetAttachParentActor())
				{
					// Check that parent has an id and class
					if (ObjToId.Contains(AttachedParent) && ObjToClass.Contains(AttachedParent))
					{
						Text.Append(Indent + TEXT("ParentClass: " + ObjToClass[AttachedParent] + ",\n"));
						Text.Append(Indent + TEXT("ParentId: " + ObjToId[AttachedParent] + ",\n"));
					}
				}
			}
			else if (UStaticMeshComponent* ObjAsComp = Cast<UStaticMeshComponent>(CurrObj))
			{
				FVector Loc = ObjAsComp->GetComponentLocation();
				FQuat Quat = ObjAsComp->GetComponentQuat();
				FString Pose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f],\n"), Loc.X, Loc.Y, Loc.Z, Quat.W, Quat.X, Quat.Y, Quat.Z);
				Text.Append(Indent + TEXT("Pose: " + Pose + "\n"));
				Text.Append(Indent + FString::Printf(TEXT("Mass: %f,\n"), ObjAsComp->GetMass()));

				if (USceneComponent* AttachmentRootComp = ObjAsComp->GetAttachmentRoot())
				{
					// Check that parent has an id and class
					if (ObjToId.Contains(AttachmentRootComp) && ObjToClass.Contains(AttachmentRootComp))
					{
						Text.Append(Indent + TEXT("ParentClass: " + ObjToClass[AttachmentRootComp] + ",\n"));
						Text.Append(Indent + TEXT("ParentId: " + ObjToId[AttachmentRootComp] + ",\n"));
					}
				}

				if (AActor* AttachmentRootActor = ObjAsComp->GetAttachmentRootActor())
				{
					// Check that parent has an id and class
					if (ObjToId.Contains(AttachmentRootActor) && ObjToClass.Contains(AttachmentRootActor))
					{
						Text.Append(Indent + TEXT("ParentClass: " + ObjToClass[AttachmentRootActor] + ",\n"));
						Text.Append(Indent + TEXT("ParentId: " + ObjToId[AttachmentRootActor] + ",\n"));
					}
				}

			}
			Indent.RemoveFromEnd(TEXT("  "));
			Text.Append(Indent + "},\n");
		}

		Indent.RemoveFromEnd(TEXT("  "));
		Text.Append(Indent + TEXT("]\n"));
		Text.Append(TEXT("\n---"));

		FString AbsFilePath = ExportDir + TEXT("World_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".yaml");
		FFileHelper::SaveStringToFile(Text, *AbsFilePath);
	}
}

// Called every frame
void AWorldExporter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
