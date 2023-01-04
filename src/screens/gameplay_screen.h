#pragma once

#include "screen.h"

namespace gunshot
{
	class GameplayScreen : public Scene::Tappable<Screen>
	{
	public:
		GameplayScreen(); 

	private:
		static const auto inline StepHeight = 18.0f;

		struct Area
		{
			std::vector<std::shared_ptr<Shared::PhysHelpers::Entity>> steps;
			bool direction;

			inline auto getFloor() const { return steps.at(0); }
			inline auto getRoof() const { return steps.at(steps.size() - 1); }
		};

		static const int inline WorldCategory = 1 << 0; // dont need to set up for every panel and npc, they already have this flag
		static const int inline PlayerCategory = 1 << 1;
		static const int inline BulletCategory = 1 << 2;
		static const int inline EnemyCategory = 1 << 3;

	private:
		Area createArea(float floor_height, float start_y_pos, bool direction, std::optional<float> prev_roof_space = std::nullopt);
		void spawnArea();
		void removeArea(int index);
		std::shared_ptr<Shared::PhysHelpers::Entity> createPlayer();
		void spawnEnemy();
		void shoot(const glm::vec2& dir);
		void removeBottomAreas();
		void spawnTopAreas();

	private:
		std::shared_ptr<Shared::PhysHelpers::World> mWorld;
		std::shared_ptr<Shared::PhysHelpers::Entity> mPlayer;
		std::shared_ptr<Shared::PhysHelpers::Entity> mEnemy;
		bool mTapped = false;

	private:
		std::map<int, Area> mAreas;
		int mAreaIndex = 0;
		float mColor;
	};
}