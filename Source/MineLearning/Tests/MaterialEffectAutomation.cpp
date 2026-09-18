#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MineLearning/Effects/MaterialEffectLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialEffectPreservationTest, "MineLearning.GurenQ.MaterialPreservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialEffectPreservationTest::RunTest(const FString& Parameters)
{
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>();
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	UMaterialInterface* Armor = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/MineLearning/Characters/OreBuddy/Materials/M_OB07_Armor.M_OB07_Armor"));
	if (!TestNotNull(TEXT("Production armor material"), Armor))
	{
		return false;
	}
	Mesh->SetMaterial(0, Armor);
	UMaterialInstanceDynamic* Effect = UMaterialEffectLibrary::CreateIsolatedMaterialInstance(Mesh, 0);
	if (!TestNotNull(TEXT("Effect instance"), Effect))
	{
		return false;
	}
	TestTrue(TEXT("Preserves the original instance parent and static permutation"), Effect->Parent == Armor);
	FLinearColor OriginalColor;
	FLinearColor EffectColor;
	TestTrue(TEXT("Original armor exposes its actual color"), Armor->GetVectorParameterValue(TEXT("DiffuseColor"), OriginalColor));
	TestTrue(TEXT("Effect retains the same color parameter"), Effect->GetVectorParameterValue(TEXT("DiffuseColor"), EffectColor));
	TestTrue(TEXT("Armor color is unchanged"), OriginalColor.Equals(EffectColor));

	UMaterialInstanceDynamic* GameplayMaterial = UMaterialInstanceDynamic::Create(Armor, Mesh);
	const FLinearColor RuntimeColor(0.21f, 0.43f, 0.65f, 1.f);
	GameplayMaterial->SetVectorParameterValue(TEXT("DiffuseColor"), RuntimeColor);
	GameplayMaterial->SetScalarParameterValue(TEXT("Shininess"), 37.f);
	Mesh->SetMaterial(0, GameplayMaterial);
	Effect = UMaterialEffectLibrary::CreateIsolatedMaterialInstance(Mesh, 0);
	TestTrue(TEXT("An existing gameplay MID is never reused"), Effect && Effect != GameplayMaterial);
	if (!Effect)
	{
		return false;
	}
	TestTrue(TEXT("Existing MID uses its constant parent, not a MID parent"), Effect->Parent == Armor);
	Effect->GetVectorParameterValue(TEXT("DiffuseColor"), EffectColor);
	TestTrue(TEXT("Runtime color override is copied"), RuntimeColor.Equals(EffectColor));
	float Shininess = 0.f;
	Effect->GetScalarParameterValue(TEXT("Shininess"), Shininess);
	TestEqual(TEXT("Runtime scalar override is copied"), Shininess, 37.f);
	Effect->SetVectorParameterValue(TEXT("DiffuseColor"), FLinearColor::Red);
	Effect->SetScalarParameterValue(TEXT("DissolveProgress"), 0.8f);
	GameplayMaterial->GetVectorParameterValue(TEXT("DiffuseColor"), OriginalColor);
	TestTrue(TEXT("The gameplay MID is not polluted by effect writes"), RuntimeColor.Equals(OriginalColor));
	Mesh->SetMaterial(0, GameplayMaterial);
	TestTrue(TEXT("Cancellation can restore the exact original object"), Mesh->GetMaterial(0) == GameplayMaterial);
	UMaterialInstanceDynamic* Repeated = UMaterialEffectLibrary::CreateIsolatedMaterialInstance(Mesh, 0);
	Repeated->GetVectorParameterValue(TEXT("DiffuseColor"), EffectColor);
	TestTrue(TEXT("Repeated effects start from the original appearance"), RuntimeColor.Equals(EffectColor));
	float Progress = -1.f;
	TestTrue(TEXT("Compatible production material exposes dissolve progress"), Repeated->GetScalarParameterValue(TEXT("DissolveProgress"), Progress));
	TestEqual(TEXT("Repeated effect has no previous dissolve progress"), Progress, 0.f);

	UMaterialInterface* Ore = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_100.M_Ore_Iron_100"));
	Mesh->SetMaterial(0, Ore);
	Effect = UMaterialEffectLibrary::CreateIsolatedMaterialInstance(Mesh, 0);
	TestTrue(TEXT("Non-parameterized ore texture graph stays the original shader"), Effect && Effect->Parent == Ore);
	TestNull(TEXT("Invalid slot is rejected"), UMaterialEffectLibrary::CreateIsolatedMaterialInstance(Mesh, 2));
	TestTrue(TEXT("Invalid slot does not replace the valid material"), Mesh->GetMaterial(0) == Effect);
	TestNull(TEXT("Missing component is safe"), UMaterialEffectLibrary::CreateIsolatedMaterialInstance(nullptr, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRadiantSurfaceCoverageTest, "MineLearning.GurenQ.RadiantSurfaceCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRadiantSurfaceCoverageTest::RunTest(const FString& Parameters)
{
	// A zero-progress material can look correct while having no heat/cutout shader at all.
	// Include the mining-stage variants, not only the intact mesh's default material.
	const TCHAR* MaterialPaths[] =
	{
		TEXT("/Game/Characters/Mannequins/Materials/Instances/Quinn/MI_Quinn_01.MI_Quinn_01"),
		TEXT("/Game/Characters/Mannequins/Materials/Instances/Quinn/MI_Quinn_02.MI_Quinn_02"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_100.M_Ore_Iron_100"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_80.M_Ore_Iron_80"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_60.M_Ore_Iron_60"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_40.M_Ore_Iron_40"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_20.M_Ore_Iron_20"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_Drop_01.M_Ore_Iron_Drop_01"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Iron_Drop_02.M_Ore_Iron_Drop_02"),
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Materials/M_Ore_Damage.M_Ore_Damage")
	};
	for (const TCHAR* Path : MaterialPaths)
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path);
		if (!TestNotNull(Path, Material))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s supports surface clipping"), Path), Material->GetBlendMode() == BLEND_Masked);
#if WITH_EDITORONLY_DATA
		const UMaterialEditorOnlyData* Data = Material->GetMaterial()->GetEditorOnlyData();
		TestTrue(FString::Printf(TEXT("%s evaluates its connected heat and mask outputs"), Path),
			Data->EmissiveColor.IsConnected() && !Data->EmissiveColor.UseConstant
			&& Data->OpacityMask.IsConnected() && !Data->OpacityMask.UseConstant);
#endif
		float Supported = 0.f;
		TestTrue(FString::Printf(TEXT("%s contains the enabled radiant surface function"), Path),
			Material->GetScalarParameterValue(TEXT("RadiantSupported"), Supported) && Supported > 0.f);
		for (const FName Name : { FName(TEXT("HeatRadius")), FName(TEXT("DissolveRadius")), FName(TEXT("DissolveProgress")), FName(TEXT("HeatIntensity")), FName(TEXT("OpacityFade")) })
		{
			float Value = 0.f;
			TestTrue(FString::Printf(TEXT("%s exposes %s"), Path, *Name.ToString()), Material->GetScalarParameterValue(Name, Value));
		}
	}
	return true;
}

#endif
