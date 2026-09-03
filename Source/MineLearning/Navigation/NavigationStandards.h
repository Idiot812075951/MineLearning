#pragma once

namespace MineLearningNavigation
{
	/** Authored ground routes may use steps up to 80 cm; movement keeps 5 cm of tolerance. */
	inline constexpr float MaxGroundStepHeight = 80.0f;
	inline constexpr float CharacterStepHeight = 85.0f;

	/** One shared NavMesh must safely contain the largest current ground pawn (OreBuddy). */
	inline constexpr float SharedAgentRadius = 85.0f;
	inline constexpr float SharedAgentHeight = 190.0f;
}
