#pragma once

#include <shared/all.h>

namespace gunshot
{
	class Application : public Shared::Application
	{
	public:
		Application();
		~Application();

	private:
		void initialize();
	};
}
