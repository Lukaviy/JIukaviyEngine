export module Main;

import Application;

export {
	int main() {
		auto app = ji::Application::create({ "Editor" });

		return app->run();
	}
}