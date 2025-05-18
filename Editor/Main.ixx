module;

#include <windows.h>

export module Main;

import Application;

export {
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
		auto app = ji::Application::create({ "Editor" });

		return app->run();
	}
}
