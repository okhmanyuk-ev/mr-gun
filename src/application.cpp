#include "application.h"
#include "screens/gameplay_screen.h"

using namespace gunshot;

Application::Application() : Shared::Application(PROJECT_NAME, { Flag::Audio, Flag::Scene, Flag::Network })
{
	PLATFORM->setTitle(PRODUCT_NAME);
#if defined(PLATFORM_MAC)
	PLATFORM->resize(720, 1280);
#else
	PLATFORM->resize(360, 640);
#endif
#if defined(PLATFORM_WINDOWS)
	PLATFORM->rescale(1.5f);
#endif
	RENDERER->setVsync(true);

	PRECACHE_FONT_ALIAS("fonts/sansation.ttf", "default");

	STATS->setAlignment(Shared::StatsSystem::Align::BottomRight);

	Scene::Sprite::DefaultSampler = skygfx::Sampler::Linear;
	Scene::Sprite::DefaultTexture = TEXTURE("textures/default.png");
    Scene::Label::DefaultFont = FONT("default");
	Scene::Scrollbox::DefaultInertiaFriction = 0.05f;

	CACHE->makeAtlases();

	FRAME->addOne([this] {
		initialize();
	});
}

Application::~Application()
{
}

void Application::initialize()
{
	auto gameplay_screen = std::make_shared<GameplayScreen>();
	SCENE_MANAGER->switchScreen(gameplay_screen);
}
