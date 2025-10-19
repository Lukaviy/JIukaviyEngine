module;

#include <exception>
#include <windows.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>

export module main;

import application;

export {
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
		auto msvc_logger = std::make_shared<spdlog::logger>("msvc", std::make_shared<spdlog::sinks::msvc_sink_st>());
		spdlog::set_default_logger(std::move(msvc_logger));
		spdlog::flush_on(spdlog::level::info);

		try {
			{
				const auto app = ji::Application::create({ "Editor" });
				app->run();
			}

			spdlog::drop_all();
			return EXIT_SUCCESS;
		} catch (const std::exception& e) {
			spdlog::drop_all();
			MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_OK);
			return EXIT_FAILURE;
		}
	}
}
