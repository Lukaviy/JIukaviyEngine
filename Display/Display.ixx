module;

#include <string>
#include <glm/glm/glm.hpp>

export module Display;

import KeyId;

export namespace ji {
	struct VideMode {
		glm::uvec2 size;

		int redBits;
		int greenBits;
		int blueBits;
		int refreshRate;
	};

	struct DisplayInfo {
		using Id = KeyId<DisplayInfo>;
		std::string name;
		glm::uvec2 resolution;
		glm::uvec2 position;
		glm::uvec2 size;
		glm::uvec2 physicalSize;
		glm::fvec2 contentScale;
		VideMode videoMode;
	};
}