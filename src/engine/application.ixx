module;

#include <string>

export module application;

import pointers;

export namespace ji {
	struct ApplicationInfo {
		std::string name;
	};

	class Application {
	public:
		virtual ~Application() = default;

		static ji::unique<Application> create(ApplicationInfo&& info);

		virtual void run() = 0;
	};
}
