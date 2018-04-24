// Copyright 2017, Institute for Artificial Intelligence - University of Bremen

#include "WorldExporter.h"
#include "Tags.h"
#include "Ids.h"
#include "Conversions.h"
#include "HAL/PlatformFilemanager.h"
#include "Paths.h"
#include "Engine/StaticMeshActor.h"
#include "PhysicsEngine/PhysicsConstraintActor.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
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

	//
	TMap<UObject*, FString> ObjToClass = FTags::GetObjectsToKeyValue(GetWorld(), TEXT("SemLog"), TEXT("Class"));
	TMap<UObject*, FString> ObjToId = FTags::GetObjectsToKeyValue(GetWorld(), TEXT("SemLog"), TEXT("Id"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();


	FString ExportDir = FPaths::ProjectDir() + TEXT("/WorldExport/");
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

			Text.Append(Indent + TEXT("Class: " + ObjToClassItr.Value + TEXT(",\n")));
			if (ObjToId.Contains(CurrObj))
			{
				Text.Append(Indent + TEXT("Id: \"" + ObjToId[CurrObj] + TEXT("\",\n")));
			}
			Text.Append(Indent + TEXT("Mesh: " + ObjToClassItr.Value + TEXT(".dae,\n")));

			if (AStaticMeshActor* ObjAsActor = Cast<AStaticMeshActor>(CurrObj))
			{
				FVector Loc = FConversions::UToROS(ObjAsActor->GetActorLocation());
				FQuat Quat = FConversions::UToROS(ObjAsActor->GetActorQuat());
				FString Pose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f]"),Loc.X, Loc.Y, Loc.Z,
					Quat.W, Quat.X, Quat.Y, Quat.Z);
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
						Text.Append(Indent + TEXT("ParentId: \"" + ObjToId[AttachedParent] + "\",\n"));
						//FTransform RelativeTransform = FConversions::UToROS(
						//	ObjAsActor->GetTransform().GetRelativeTransform(AttachedParent->GetTransform()));
						//FVector RelLoc = RelativeTransform.GetLocation(); // ToMeters
						//FQuat RelQuat = RelativeTransform.GetRotation();
						//FString RelativePose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f]"), RelLoc.X, RelLoc.Y, RelLoc.Z,
						//	RelQuat.W, RelQuat.X, RelQuat.Y, RelQuat.Z);
						//Text.Append(Indent + TEXT("RelativePose: " + RelativePose + ",\n"));
					}
				}
			}
			else if (UStaticMeshComponent* ObjAsComp = Cast<UStaticMeshComponent>(CurrObj))
			{
				FVector Loc = FConversions::UToROS(ObjAsComp->GetComponentLocation());
				FQuat Quat = FConversions::UToROS(ObjAsComp->GetComponentQuat());
				FString Pose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f],\n"), Loc.X, Loc.Y, Loc.Z,
					Quat.W, Quat.X, Quat.Y, Quat.Z);
				Text.Append(Indent + TEXT("Pose: " + Pose + "\n"));
				Text.Append(Indent + FString::Printf(TEXT("Mass: %f,\n"), ObjAsComp->GetMass()));

				if (USceneComponent* AttachmentRootComp = ObjAsComp->GetAttachmentRoot())
				{
					// Check that parent has an id and class
					if (ObjToId.Contains(AttachmentRootComp) && ObjToClass.Contains(AttachmentRootComp))
					{
						Text.Append(Indent + TEXT("ParentClass: " + ObjToClass[AttachmentRootComp] + ",\n"));
						Text.Append(Indent + TEXT("ParentId: \"" + ObjToId[AttachmentRootComp] + "\",\n"));
					}
				}

				if (AActor* AttachmentRootActor = ObjAsComp->GetAttachmentRootActor())
				{
					// Check that parent has an id and class
					if (ObjToId.Contains(AttachmentRootActor) && ObjToClass.Contains(AttachmentRootActor))
					{
						Text.Append(Indent + TEXT("ParentClass: " + ObjToClass[AttachmentRootActor] + ",\n"));
						Text.Append(Indent + TEXT("ParentId: \"" + ObjToId[AttachmentRootActor] + "\",\n"));						
						//FTransform RelativeTransform = FConversions::UToROS(
						//	ObjAsComp->GetComponentTransform().GetRelativeTransform(AttachmentRootActor->GetTransform()));
						//FVector RelLoc = RelativeTransform.GetLocation();
						//FQuat RelQuat = RelativeTransform.GetRotation();
						//FString RelativePose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f]"), RelLoc.X, RelLoc.Y, RelLoc.Z,
						//	RelQuat.W, RelQuat.X, RelQuat.Y, RelQuat.Z);
						//Text.Append(Indent + TEXT("RelativePose: " + RelativePose + ",\n"));
					}
				}
			}
			Indent.RemoveFromEnd(TEXT("  "));
			Text.Append(Indent + "},\n");
		}

		// Iterate constraints, check if constraint actors have semantic class and id defined
		for (TActorIterator<APhysicsConstraintActor> ConstrActItr(GetWorld()); ConstrActItr; ++ConstrActItr)
		{
			UPhysicsConstraintComponent* ConstrComp = ConstrActItr->GetConstraintComp();

			// Check that both constrained actors have a class and id
			if (ObjToId.Contains(ConstrComp->ConstraintActor1) && ObjToClass.Contains(ConstrComp->ConstraintActor1) &&
				ObjToId.Contains(ConstrComp->ConstraintActor2) && ObjToClass.Contains(ConstrComp->ConstraintActor2))
			{
				Text.Append(Indent + "{\n");
				Indent = Indent.Append(TEXT("  "));

				Text.Append(Indent + TEXT("JointIds: [\"" + ObjToId[ConstrComp->ConstraintActor1] + TEXT(",\"")
					+ ObjToId[ConstrComp->ConstraintActor2] +  TEXT("\"],\n")));

				Text.Append(Indent + TEXT("JointClasses: [\"" + ObjToClass[ConstrComp->ConstraintActor1] + TEXT(",\"")
					+ ObjToClass[ConstrComp->ConstraintActor2] + TEXT("\"],\n")));

				FVector Loc = FConversions::UToROS(ConstrActItr->GetActorLocation());
				FQuat Quat = FConversions::UToROS(ConstrActItr->GetActorQuat());
				FString Pose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f]"), Loc.X, Loc.Y, Loc.Z,
					Quat.W, Quat.X, Quat.Y, Quat.Z);
				Text.Append(Indent + TEXT("Pose: " + Pose + ",\n"));

				//FTransform RelativeTransform = ConstrActItr->GetTransform().GetRelativeTransform(ConstrComp->ConstraintActor1->GetTransform());
				//FVector RelLoc = FConversions::UToROS(RelativeTransform.GetLocation());
				//FQuat RelQuat = FConversions::UToROS(RelativeTransform.GetRotation());
				//FString RelativePose = FString::Printf(TEXT("[%f,%f,%f,%f,%f,%f,%f]"), RelLoc.X, RelLoc.Y, RelLoc.Z,
				//	RelQuat.W, RelQuat.X, RelQuat.Y, RelQuat.Z);
				//Text.Append(Indent + TEXT("RelativePose: " + RelativePose + ",\n"));


				// Check constraint type
				if (ConstrComp->ConstraintInstance.GetLinearXMotion() != ELinearConstraintMotion::LCM_Locked ||
					ConstrComp->ConstraintInstance.GetLinearYMotion() != ELinearConstraintMotion::LCM_Locked ||
					ConstrComp->ConstraintInstance.GetLinearZMotion() != ELinearConstraintMotion::LCM_Locked ||
					ConstrComp->ConstraintInstance.GetAngularSwing1Motion() != ELinearConstraintMotion::LCM_Locked ||
					ConstrComp->ConstraintInstance.GetAngularSwing2Motion() != ELinearConstraintMotion::LCM_Locked ||
					ConstrComp->ConstraintInstance.GetAngularTwistMotion() != ELinearConstraintMotion::LCM_Locked)
				{
					if (ConstrComp->ConstraintInstance.GetLinearXMotion() != ELinearConstraintMotion::LCM_Locked)
					{
						Text.Append(Indent + TEXT("Type: \"Prismatic\",\n"));
						Text.Append(Indent + TEXT("Axis: [1 0 0],\n"));
						Text.Append(Indent + TEXT("Limit: [0," + 
							FString::SanitizeFloat(FConversions::CmToM(ConstrComp->ConstraintInstance.GetLinearLimit())) + "],\n"));
					}
					else if (ConstrComp->ConstraintInstance.GetLinearYMotion() != ELinearConstraintMotion::LCM_Locked)
					{
						Text.Append(Indent + TEXT("Type: \"Prismatic\",\n"));
						Text.Append(Indent + TEXT("Axis: [0 1 0],\n"));
						Text.Append(Indent + TEXT("Limit: [0," + 
							FString::SanitizeFloat(FConversions::CmToM(ConstrComp->ConstraintInstance.GetLinearLimit())) + "],\n"));
					}
					else if (ConstrComp->ConstraintInstance.GetLinearZMotion() != ELinearConstraintMotion::LCM_Locked)
					{
						Text.Append(Indent + TEXT("Type: \"Prismatic\",\n"));
						Text.Append(Indent + TEXT("Axis: [0 0 1],\n"));
						Text.Append(Indent + TEXT("Limit: [0," + 
							FString::SanitizeFloat(FConversions::CmToM(ConstrComp->ConstraintInstance.GetLinearLimit())) + "],\n"));
					}
					else if (ConstrComp->ConstraintInstance.GetAngularSwing1Motion() != EAngularConstraintMotion::ACM_Locked)
					{
						Text.Append(Indent + TEXT("Type: \"Revolute\",\n"));
						Text.Append(Indent + TEXT("Axis: [0 0 1],\n"));
						Text.Append(Indent + TEXT("Limit: [0," + 
							FString::SanitizeFloat(FMath::DegreesToRadians(ConstrComp->ConstraintInstance.GetAngularSwing1Limit() * 2.f)) + "],\n"));
					}
					else if (ConstrComp->ConstraintInstance.GetAngularSwing2Motion() != EAngularConstraintMotion::ACM_Locked)
					{
						Text.Append(Indent + TEXT("Type: \"Revolute\",\n"));
						Text.Append(Indent + TEXT("Axis: [0 1 0],\n"));
						Text.Append(Indent + TEXT("Limit: [0," +
							FString::SanitizeFloat(FMath::DegreesToRadians(ConstrComp->ConstraintInstance.GetAngularSwing2Limit() * 2.f)) + "],\n"));
					}
					else if (ConstrComp->ConstraintInstance.GetAngularTwistMotion() != EAngularConstraintMotion::ACM_Locked)
					{
						Text.Append(Indent + TEXT("Type: \"Revolute\",\n"));
						Text.Append(Indent + TEXT("Axis: [1 0 0],\n"));
						Text.Append(Indent + TEXT("Limit: [0," +
							FString::SanitizeFloat(FMath::DegreesToRadians(ConstrComp->ConstraintInstance.GetAngularTwistLimit() * 2.f)) + "],\n"));
					}
				}
				else
				{
					// All motions are locked, fixed joint
					Text.Append(Indent + TEXT("Type: \"Fixed\",\n"));
				}


				Indent.RemoveFromEnd(TEXT("  "));
				Text.Append(Indent + "},\n");
			}
		}

		Indent.RemoveFromEnd(TEXT("  "));
		Text.Append(Indent + TEXT("]\n"));
		Text.Append(TEXT("\n---"));

		FString AbsFilePath = ExportDir + TEXT("World_") + FIds::NewGuidInBase64Url() + TEXT(".yaml");
		FFileHelper::SaveStringToFile(Text, *AbsFilePath);
	}
}

// Called every frame
void AWorldExporter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
