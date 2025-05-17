module;

#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>

export module ApplicationWindows;

import Application;
import Pointers;

namespace ji {
	class ApplicationWindows : public Application {
	public:
		void run() override {
			auto window = glfw::Window{ 800, 600, m_applicationInfo.name.data() };

			while (!window.shouldClose()) {
				window.swapBuffers();
				glfw::pollEvents();
			}
		}

	private:
		ApplicationWindows(glfw::GlfwLibrary lib, ApplicationInfo&& info) : m_glfw(std::move(lib)), m_applicationInfo(std::move(info)) {

		}

		glfw::GlfwLibrary m_glfw;
		ApplicationInfo m_applicationInfo;
	};

	unique<Application> Application::create(ApplicationInfo&& info) {
		return make_unique<ApplicationWindows>(glfw::init());
	}
}
