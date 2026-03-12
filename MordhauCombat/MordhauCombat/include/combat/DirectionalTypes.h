#pragma once

#include <cstdint>

namespace MC::DIRECTIONAL
{
	enum class DirectionBucket : std::uint8_t
	{
		Center = 0,
		Top,
		TopRight,
		Right,
		BottomRight,
		Bottom,
		BottomLeft,
		Left,
		TopLeft
	};

	struct CursorVectorState
	{
		float x{ 0.0f };
		float y{ 0.0f };
		float magnitude{ 0.0f };
		DirectionBucket bucket{ DirectionBucket::Center };
	};

	struct LatchedAttackState
	{
		DirectionBucket bucket{ DirectionBucket::Center };
		bool power{ false };
		float startWorldTime{ 0.0f };
		bool active{ false };
	};
}
