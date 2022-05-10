export module DisplayManager;

import <vector>;
import Display;

export namespace ji {
	class DisplayManager {
	public:
		virtual const std::vector<DisplayInfo>& getDisplays() const = 0;
	};
}
