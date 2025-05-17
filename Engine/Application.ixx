export module Application;

import <string>;
import Pointers;

export namespace ji {
	struct ApplicationInfo {
		std::string name;
	};

	class Application {
	public:
		virtual ~Application() = default;

		static unique<Application> create(ApplicationInfo&& info);

		virtual void run() = 0;
	};
}
