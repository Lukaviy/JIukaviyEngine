module;

#include <glm/glm.hpp>

export module Display;

import KeyId;
export import <string>;
export import <vector>;
import <algorithm>;

export namespace ji {
	struct WorkArea {
		glm::uvec2 position;
		glm::uvec2 size;
	};

	struct VideoMode {
		using Id = ji::KeyId<VideoMode, const void*>;
		Id id;
		glm::uvec2 size;
		glm::uvec3 colorBits;
		int refreshRate;
	};

	struct DisplayInfo {
		using Id = KeyId<DisplayInfo, void*>;

		/*DisplayInfo(Id id, std::string&& name, WorkArea&& workArea, glm::uvec2 physicalSize, glm::fvec2 contentScale, VideoMode&& currentVideoMode, std::vector<VideoMode>&& videoModes)
			: id (id), name(std::move(name)), workArea(std::move(workArea)), physicalSize(physicalSize), contentScale(contentScale), currentVideoMode(std::move(currentVideoMode)), videoModes(std::move(videoModes)) {}*/

		Id id;
		std::string name;
		WorkArea workArea;
		glm::uvec2 physicalSize;
		glm::fvec2 contentScale;
		VideoMode currentVideoMode;
		std::vector<VideoMode> videoModes;
	};
}