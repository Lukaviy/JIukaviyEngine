module;

#include <string>

export module Application;

import Pointers;

export namespace ji {
	struct ApplicationInfo {
		std::string name;
	};

	class Application {
	public:
		virtual ~Application() = default;

		static unique<Application> create(ApplicationInfo&& info);

		virtual int run() = 0;
	};
}
