#include "gameplay_screen.h"

using namespace gunshot;

GameplayScreen::GameplayScreen()
{
	mWorld = std::make_shared<Shared::PhysHelpers::World>();
	mWorld->setAnchor(0.5f);
	mWorld->setPivot(0.5f);
	getContent()->attach(mWorld);

	runAction(Actions::Collection::ExecuteInfinite([this](auto delta) {
		auto src = mWorld->getPosition();
		auto dst = -mPlayer->getPosition();
		GAME_STATS("pos", fmt::format("{:.0f} {:.0f}", dst.x, dst.y));
		GAME_STATS("areas", mAreas.size());
		dst.x = 0.0f;
		mWorld->setPosition(Common::Helpers::SmoothValueAssign(src, dst, delta, 0.0125f));
		
		removeBottomAreas();
		spawnTopAreas();
	}));

	mColor = glm::linearRand(0.0f, 360.0f);

	spawnArea();
	
	mPlayer = createPlayer();
	mWorld->attach(mPlayer);
	mPlayer->setFilterCategoryBits(PlayerCategory);

	auto area = mAreas.at(mAreaIndex);
	auto floor = area.getFloor();
	auto space = floor->getWidth() - area.steps.at(1)->getWidth();
	auto pos_x = (floor->getWidth() * 0.5f) - space * 0.5f;
	auto pos_y = floor->getY();

	mPlayer->setPosition({ pos_x, pos_y });
	
	spawnEnemy();
	
	setTapCallback([this](const glm::vec2& tap_pos) {
		auto projected_player_pos = mPlayer->project(mPlayer->getSize() * 0.5f);
		auto dir = glm::normalize(tap_pos - projected_player_pos);
		shoot(dir);
	});
}

GameplayScreen::Area GameplayScreen::createArea(float floor_height, float start_y_pos, bool direction, std::optional<float> prev_roof_space)
{
	auto createStaticEnt = [] {
		auto result = std::make_shared<Shared::PhysHelpers::Entity>();
		result->setType(Shared::PhysHelpers::Entity::Type::Static);
		result->setPivot({ 0.5f, 0.0f });
		result->setVisible(false);

		auto rect = std::make_shared<Scene::Rectangle>();
		rect->setStretch(1.0f);
		result->attach(rect);
		return result;
	};

	Area result;
	result.direction = direction;

	auto floor = createStaticEnt();
	floor->setPosition({ 0.0f, start_y_pos });
	floor->setSize({ 360.0f, floor_height });
	result.steps.push_back(floor);

	auto floor_space = glm::linearRand(90.0f, 270.0f);
		
	if (prev_roof_space.has_value())
		floor_space = glm::max(floor_space, prev_roof_space.value());
		
	auto roof_space = glm::linearRand(90.0f, floor->getWidth() - floor_space);

	floor_space -= glm::mod(floor_space, StepHeight);
	roof_space -= glm::mod(roof_space, StepHeight);

	auto step_width = floor->getWidth() - floor_space;
	auto step_pos_x = (step_width * 0.5f) - (floor->getWidth() * 0.5f);
	auto step_pos_y = start_y_pos - StepHeight;

	auto step = createStaticEnt();
	step->setPosition({ direction ? step_pos_x : -step_pos_x, step_pos_y });
	step->setSize({ step_width, StepHeight });
	result.steps.push_back(step);

	while (step_width > roof_space)
	{
		step_width -= StepHeight;
		step_pos_x -= StepHeight * 0.5f;
		step_pos_y -= StepHeight;

		auto steps = createStaticEnt();
		steps->setPosition({ direction ? step_pos_x : -step_pos_x, step_pos_y });
		steps->setSize({ step_width, StepHeight });
		result.steps.push_back(steps);
	}

	floor->runAction(Actions::Collection::ExecuteInfinite([this, result] {
		if (!result.getFloor()->isTransformReady())
			return;

		auto projected_floor_pos = result.getFloor()->project({ 0.0f, 0.0f });
		auto value = projected_floor_pos.y / PLATFORM->getHeight();
		value = glm::clamp(value);
		auto color = glm::rgbColor(glm::vec3(mColor, 0.66f, value));

		for (auto step : result.steps)
		{
			step->setVisible(true);
			Shared::SceneHelpers::RecursiveColorSet(step, { color, 1.0f });
		}
	}));

	return result;
}

void GameplayScreen::spawnArea()
{
	float floor_height = 400.0f;
	float floor_pos_y = 0.0f;
	bool direction = true;
	std::optional<float> prev_roof_space;

	int spawn_index = 0;

	if (!mAreas.empty())
	{
		auto [top_index, top_area] = *mAreas.rbegin();
		auto floor = top_area.getFloor();
		auto roof = top_area.getRoof();

		spawn_index = top_index + 1;
		floor_height = glm::abs(floor->getY() - roof->getY());
		floor_pos_y = roof->getY();
		direction = !top_area.direction;
		prev_roof_space = roof->getWidth();
	}

	auto area = createArea(floor_height, floor_pos_y, direction, prev_roof_space);
	mAreas.insert({ spawn_index, area });

	for (auto step : area.steps)
	{
		mWorld->attach(step, Scene::Node::AttachDirection::Front);
		step->setFilterLayer(spawn_index + 1);
	}
}

void GameplayScreen::removeArea(int index)
{
	auto area = mAreas.at(index);
	for (auto step : area.steps)
	{
		mWorld->detach(step);
	}
	mAreas.erase(index);
}

std::shared_ptr<Shared::PhysHelpers::Entity> GameplayScreen::createPlayer()
{
	auto result = std::make_shared<Shared::PhysHelpers::Entity>();
	result->setType(Shared::PhysHelpers::Entity::Type::Static);
	result->setFixedRotation(true);
	result->setPosition({ 0.0f, 0.0f });
	result->setSize({ 20.0f, 40.0f });
	result->setPivot({ 0.5f, 1.0f });

	auto body = std::make_shared<Scene::Rectangle>();
	body->setSize({ 15.0f, 40.0f });
	body->setAnchor(0.5f);
	body->setPivot(0.5f);
	body->setColor(Graphics::Color::ToNormalized(10, 31, 45));
	result->attach(body);

	auto head = std::make_shared<Scene::Rectangle>();
	head->setSize({ 20.0f, 15.0f });
	head->setAnchor({ 0.5f, 0.0f });
	head->setPivot({ 0.5f, 0.0 });
	result->attach(head);

	return result;
}

void GameplayScreen::spawnEnemy()
{
	mEnemy = createPlayer();
	mWorld->attach(mEnemy);
	mEnemy->setFilterCategoryBits(EnemyCategory);
	mEnemy->setFilterLayer(mAreaIndex + 1);

	auto area = mAreas.at(mAreaIndex);
	auto direction = area.direction;
	
	auto roof = area.getRoof();

	auto src_pos = roof->getPosition();
	src_pos.x += roof->getWidth() * 0.5f * (direction ? -1.0f : 1.0f);
	src_pos.x -= mEnemy->getWidth() * (direction ? 1.0f : -1.0f);

	auto dst_pos = roof->getPosition();
	dst_pos.x += roof->getWidth() * 0.5f * (direction ? 1.0f : -1.0f);
	dst_pos.x -= mEnemy->getWidth() * (direction ? 1.0f : -1.0f);

	mEnemy->setPosition(src_pos);
	mEnemy->runAction(Actions::Collection::ChangePosition(mEnemy, dst_pos, 0.5f));

	/*auto door = std::make_shared<Scene::Rectangle>();
	door->setHeight(mEnemy->getHeight() + 8.0f);
	door->setPivot({ direction ? 0.0f : 1.0f, 1.0f });
	door->setWidth(32.0f);
	door->setAlpha(0.5f);
	door->setScale({ 0.0f, 1.0f });
	door->setY(roof->getY());
	door->setX(roof->getX() + (roof->getWidth() * 0.5f * (direction ? -1.0f : 1.0f)));
	door->setVerticalOrigin(2.0f);
	auto edge = direction ? Scene::Rectangle::Edge::Right : Scene::Rectangle::Edge::Left;
	door->getEdgeColor(edge)->setAlpha(0.0f);
	mWorld->attach(door);

	
	door->runAction(Actions::Collection::MakeSequence(
		Actions::Collection::ChangeHorizontalScale(door, 1.0f, 0.25f),
		Actions::Collection::ChangePosition(mEnemy, dst_pos, 0.5f),
		Actions::Collection::ChangeHorizontalScale(door, 0.0f, 0.25f)
	));*/
}

void GameplayScreen::shoot(const glm::vec2& dir)
{
	auto bullet = std::make_shared<Shared::PhysHelpers::Entity>();
	bullet->setType(Shared::PhysHelpers::Entity::Type::Dynamic);
	bullet->setSize({ 4.0f, 4.0f });
	bullet->setPivot(0.5f);

	auto rect = std::make_shared<Scene::Rectangle>();
	rect->setStretch(1.0f);
	rect->setAlpha(0.5f);
	bullet->attach(rect);
		
	auto bullet_pos = mPlayer->getPosition() - (mPlayer->getSize() * mPlayer->getPivot()) + (mPlayer->getSize() * 0.5f);
	bullet->setPosition({ bullet_pos.x, bullet_pos.y });
	bullet->setBullet(true);
		
	mWorld->attach(bullet);

	auto speed = 0.1f;
	auto impulse = dir * speed;
	bullet->applyLinearImpulseToCenter(impulse, true);
	bullet->setGravityScale(0.0f);
		
	bullet->setFilterCategoryBits(BulletCategory);
	bullet->setFilterMaskBits(WorldCategory | EnemyCategory);
	bullet->setFilterLayer(mAreaIndex + 1);

	bullet->runAction(Actions::Collection::Delayed(3.0f, 
		Actions::Collection::MakeSequence(
			Actions::Collection::ChangeScale(bullet, { 0.0f, 0.0f }, 0.25f, Easing::CubicInOut),
			Actions::Collection::Kill(bullet))
		)
	);

	bullet->setContactCallback([this, bullet](Shared::PhysHelpers::Entity& entity) {
		if (entity.getFilterCategoryBits() != EnemyCategory)
			return;

		auto area = mAreas.at(mAreaIndex);
		auto movement_action = Actions::Collection::MakeSequence();

		const auto step_duration = 0.2f;

		{
			auto pos_x = area.steps.at(1)->getX() + (area.steps.at(1)->getWidth() * 0.5f);

			if (!area.direction)
			{
				pos_x -= area.steps.at(1)->getWidth();
				pos_x -= StepHeight;
			}
			else
			{
				pos_x += StepHeight;
			}

			movement_action->add(Actions::Collection::ChangeHorizontalPosition(mPlayer, pos_x, step_duration * 1.5f));
		}

		for (int i = 1; i < area.steps.size(); i++)
		{
			auto step = area.steps.at(i);

			auto pos_x = step->getX() + (step->getWidth() * 0.5f);
			auto pos_y = step->getY();
			auto pos_y_high = pos_y - (StepHeight * 0.6f);

			if (!area.direction)
				pos_x -= step->getWidth();

			movement_action->add(Actions::Collection::MakeParallel(
				Actions::Collection::ChangeHorizontalPosition(mPlayer, pos_x, step_duration),
				Actions::Collection::MakeSequence(
					Actions::Collection::ChangeVerticalPosition(mPlayer, pos_y_high, step_duration * 0.5f, Easing::CubicOut),
					Actions::Collection::ChangeVerticalPosition(mPlayer, pos_y, step_duration * 0.5f, Easing::CubicIn)
				)
			));
		}
		{
			auto pos_x = area.getRoof()->getX();
			auto pos_y = area.getRoof()->getY();
			auto pos_y_high = pos_y - (StepHeight * 0.6f);

			movement_action->add(Actions::Collection::MakeParallel(
				Actions::Collection::ChangeHorizontalPosition(mPlayer, pos_x, step_duration),
				Actions::Collection::MakeSequence(
					Actions::Collection::ChangeVerticalPosition(mPlayer, pos_y_high, step_duration * 0.5f, Easing::CubicOut),
					Actions::Collection::ChangeVerticalPosition(mPlayer, pos_y, step_duration * 0.5f, Easing::CubicIn)
				)
			));
		}

		mPlayer->runAction(Actions::Collection::MakeSequence(
			Actions::Collection::Kill(bullet),
			Actions::Collection::Kill(mEnemy),
			std::move(movement_action),
			Actions::Collection::Execute([this] {
				mAreaIndex += 1;
				spawnEnemy();
			})
		));
	});
}

void GameplayScreen::removeBottomAreas()
{
	auto [bottom_index, bottom_area] = *mAreas.begin();

	if (bottom_index >= mAreaIndex)
		return;

	auto bottom_roof = bottom_area.getRoof();

	if (!bottom_roof->isTransformReady())
		return;

	auto projected_bottom_roof_pos = bottom_roof->project({ 0.0f, 0.0f });

	if (projected_bottom_roof_pos.y >= PLATFORM->getHeight())
		removeArea(bottom_index);
}

void GameplayScreen::spawnTopAreas()
{
	auto [top_index, top_area] = *mAreas.rbegin();
	auto top_floor = top_area.getFloor();

	if (!top_floor->isTransformReady())
		return;

	auto projected_top_floor_pos = top_floor->project({ 0.0f, 0.0f });

	if (projected_top_floor_pos.y > 0.0f)
		spawnArea();
}
