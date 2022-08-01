export module DisplayManager;

import <vector>;
import <variant>;
export import Display;
export import Window;
export import Pointers;
export import Result;

export namespace ji {
	namespace errors {
		struct TooManyWindows {};
		struct InternalError {
			std::string error;
		};

		using WindowsCreateError = std::variant<TooManyWindows, InternalError>;
	}

	class DisplayManager {
	public:
		virtual ~DisplayManager() = default;
		virtual const std::vector<DisplayInfo>& getDisplays() const = 0;
		virtual ji::result<ji::unique<ji::Window>, errors::WindowsCreateError> createWindow() = 0;
	};
}
