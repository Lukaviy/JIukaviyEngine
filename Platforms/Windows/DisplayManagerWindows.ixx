module;

#include <glfw/include/GLFW/glfw3.h>
#include <expected/include/tl/expected.hpp>

export module DisplayManagerWindows;

import <vector>;
import <variant>;
import DisplayManager;

export namespace ji {
	namespace error {
		struct GlfwIsNotInitialized {};

		struct UnexpectedError {
			std::string description;
			int code;
		};

		using DisplayManagerWindowsError = std::variant<GlfwIsNotInitialized, UnexpectedError>;
	}

	class DisplayManagerWindows : public DisplayManager {
	public:
		static tl::expected<DisplayManagerWindows, error::DisplayManagerWindowsError> create() {
			int count;
			auto monitors = glfwGetMonitors(&count);

			if (!monitors) {
				auto code = glfwGetError(nullptr);

				if (code == GLFW_NOT_INITIALIZED) {
					return tl::unexpected(error::GlfwIsNotInitialized{});
				}

				return tl::unexpected(error::UnexpectedError{ code });
			}

			for (auto i = 0u; i < count; ++i) {
			}
		}

		DisplayManagerWindows(std::vector<DisplayInfo> m_displays) {

		}

		const std::vector<DisplayInfo>& getDisplays() const override {
			return m_displays;
		}

	private:
		std::vector<DisplayInfo> m_displays;

		static tl::expected<DisplayInfo, error::DisplayManagerWindowsError> getDisplayInfo(GLFWmonitor* monitor) {
			DisplayInfo displayInfo;

			int xpos, ypos, width, height;

			glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);

			int widthMM, heightMM;

			glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);

			auto namePtr = glfwGetMonitorName(monitor);

			auto name = namePtr ? std::string(namePtr) : std::string();

			float xscale, yscale;

			glfwGetMonitorContentScale(monitor, &xscale, &yscale);

			auto videoModePtr = glfwGetVideoMode(monitor);

			if (!videoModePtr) {
				return tl::unexpected{ error::UnexpectedError(glfwGetError(nullptr)) };
			}

			return DisplayInfo{
				.name = std::move(name),
				.resolution = 
			};
		}
	};
}