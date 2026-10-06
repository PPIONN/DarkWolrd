// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DarkWolrd : ModuleRules
{
	public DarkWolrd(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" , "UMG" });

		// Niagara: 저주 상태 VFX(BaseMonster::CurseVFX) 스폰에 사용. .cpp에서만 완전한 타입이 필요해 Private로 충분함
		// AIModule: 소환수 AI(USummonAIComponent)가 AAIController::MoveToActor 등을 사용
		// NavigationSystem: 소환수 배회 지점을 내비메시 위의 갈 수 있는 곳에서 고를 때 사용
		// AssetRegistry: 테스트 명령 DebugGiveCard가 몬스터 블루프린트를 이름으로 찾을 때 사용
		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara", "AIModule", "NavigationSystem", "AssetRegistry" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
