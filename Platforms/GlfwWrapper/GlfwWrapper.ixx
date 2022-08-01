module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

export module GlfwWrapper;

export import <string>;
export import <variant>;
export import <vector>;
export import <span>;
export import Result;
export import KeyId;
export import Display;
import <optional>;

export namespace glfw::errors {
	struct Error {
		std::string description;
		int code;
	};
}

namespace glfw {
	[[nodiscard]] std::optional<errors::Error> getError() noexcept {
		const char* description;
		auto code = ::glfwGetError(&description);

		if (code == GLFW_NO_ERROR) {
			return std::nullopt;
		}

		return errors::Error{
			.description = description,
			.code = code
		};
	}
}

export namespace glfw {
	class Glfw {
	public:
		Glfw(const Glfw&) = delete;

		Glfw(Glfw&& e) : m_owner(std::exchange(e.m_owner, false)) {}

		Glfw& operator=(const Glfw& e) = delete;

		Glfw& operator=(Glfw&& e) {
			e.m_owner = std::exchange(e.m_owner, false);
			return *this;
		}

		~Glfw() {
			if (m_owner) {
				::glfwTerminate();
			}
		}

		[[nodiscard]] static ji::result<Glfw, errors::Error> create() noexcept {
			if (::glfwInit() == GLFW_FALSE) {
				return ji::error(getError().value());
			}

			return Glfw();
		}

	private:
		Glfw() : m_owner(true) {}

		bool m_owner;
	};

	[[nodiscard]] ji::result<Glfw, errors::Error> init() noexcept {
		return Glfw::create();
	}

	ji::result<std::span<GLFWmonitor*>, errors::Error> getMonitors() noexcept {
		int count;
		auto monitors = ::glfwGetMonitors(&count);

		if (!monitors) {
			return ji::error(getError().value());
		}

		return std::span<GLFWmonitor*>(monitors, count);
	}

	ji::result<ji::WorkArea, errors::Error> getMonitorWorkArea(GLFWmonitor* monitor) noexcept {
		int xpos, ypos, width, height;

		glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);

		if (xpos == 0 && ypos == 0 && width == 0 && height == 0) {
			return ji::error(getError().value());
		}

		return ji::WorkArea{
			.position = { xpos, ypos },
			.size = { width, height }
		};
	}

	ji::result<glm::uvec2, errors::Error> getMonitorPhysicalSize(GLFWmonitor* monitor) noexcept {
		int widthMM, heightMM;

		glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);

		if (widthMM == 0 && heightMM == 0) {
			return ji::error(getError().value());
		}

		return glm::uvec2{ widthMM, heightMM };
	}

	ji::result<std::string, errors::Error> getMonitorName(GLFWmonitor* monitor) noexcept {
		auto namePtr = glfwGetMonitorName(monitor);

		if (!namePtr) {
			return ji::error(getError().value());
		}

		return std::string(namePtr);
	}

	ji::result<glm::fvec2, errors::Error> getMonitorContentScale(GLFWmonitor* monitor) noexcept {
		float xscale, yscale;

		glfwGetMonitorContentScale(monitor, &xscale, &yscale);

		return glm::fvec2{ xscale, yscale };
	}

	ji::result<ji::VideoMode, errors::Error> getVideoMode(GLFWmonitor* monitor) noexcept {
		auto videoModePtr = glfwGetVideoMode(monitor);

		if (!videoModePtr) {
			return ji::error{ getError().value() };
		}

		return ji::VideoMode{
			.id = { static_cast<const void*>(videoModePtr) },
			.size = { videoModePtr->width, videoModePtr->height },
			.colorBits = { videoModePtr->redBits, videoModePtr->greenBits, videoModePtr->blueBits },
			.refreshRate = { videoModePtr->refreshRate }
		};
	}

	ji::result<std::vector<ji::VideoMode>, errors::Error> getVideoModes(GLFWmonitor* monitor) noexcept {
		int count = 0;

		auto videoModePtr = glfwGetVideoModes(monitor, &count);

		if (!videoModePtr) {
			return ji::error{ getError().value() };
		}

		std::vector<ji::VideoMode> res;
		res.reserve(count);

		for (auto i = 0; i < count; ++i) {
			res.push_back(ji::VideoMode{
				.id = { static_cast<const void*>(videoModePtr) },
				.size = { videoModePtr->width, videoModePtr->height },
				.colorBits = { videoModePtr->redBits, videoModePtr->greenBits, videoModePtr->blueBits },
				.refreshRate = { videoModePtr->refreshRate }
			});
		}

		return res;
	}


}
