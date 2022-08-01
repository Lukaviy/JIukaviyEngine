module;

#include <GLFW/glfw3.h>

export module DisplayManagerWindows;

export import <vector>;
export import <variant>;
export import DisplayManager;
import <ranges>;
import GlfwWrapper;
import RangeHelpers;
import Result;

export namespace ji {
	namespace errors {
		using DisplayManagerWindowsError = glfw::errors::Error;
	}

	class DisplayManagerWindows : public DisplayManager {
	public:
		static ji::result<DisplayManagerWindows, errors::DisplayManagerWindowsError> create() {
			co_await glfw::init();

			auto monitors = co_await glfw::getMonitors();

			auto displayResults = monitors
				| std::ranges::views::transform([](GLFWmonitor* monitor) { return getDisplayInfo(monitor); })
				| ji::ranges::to<std::vector>{};

			if (auto errors = displayResults | std::ranges::views::filter([](auto&& e) { return !(bool)e; }); !errors.empty()) {
				co_return ji::error(std::move(errors.front()).error());
			}

			auto displays = displayResults | std::ranges::views::transform([](auto&& e) { return std::move(e).value(); }) | ji::ranges::to<std::vector>{};

			co_return DisplayManagerWindows(std::move(displays));
		}

		DisplayManagerWindows(std::vector<DisplayInfo> displays) : m_displays(std::move(displays)) {

		}

		~DisplayManagerWindows() override {
			glfw::terminate();
		}

		const std::vector<DisplayInfo>& getDisplays() const override {
			return m_displays;
		}

	private:
		std::vector<DisplayInfo> m_displays;

		static ji::result<DisplayInfo, errors::DisplayManagerWindowsError> getDisplayInfo(GLFWmonitor* monitor) {
			auto id = DisplayInfo::Id{ static_cast<void*>(monitor) };
			auto name = co_await glfw::getMonitorName(monitor);
			auto workArea = co_await glfw::getMonitorWorkArea(monitor);
			auto physicalSize = co_await glfw::getMonitorPhysicalSize(monitor);
			auto contentScale = co_await glfw::getMonitorContentScale(monitor);
			auto currentVideoMode = co_await glfw::getVideoMode(monitor);
			auto videoModes = co_await glfw::getVideoModes(monitor);

			co_return ji::DisplayInfo{
				.id = std::move(id),
				.name = std::move(name),
				.workArea = std::move(workArea),
				.physicalSize = std::move(physicalSize),
				.contentScale = std::move(contentScale),
				.currentVideoMode = std::move(currentVideoMode),
				.videoModes = std::move(videoModes),
			};
		}
	};
}