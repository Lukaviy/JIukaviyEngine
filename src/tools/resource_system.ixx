module;

#include <vector>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>

export module resource_system;

export namespace ji {
	class ResourceSystem {
	public:
		static std::vector<std::byte> loadFile(const std::filesystem::path& path) {
			std::ifstream file(path.native(), std::ios::ate | std::ios::binary);

			if (!file) {
				throw std::runtime_error(fmt::format("Failed to open file: {}", path.generic_string()));
			}

			const auto size = file.tellg();
			file.seekg(0);

			std::vector<std::byte> buffer(size);
			file.read(reinterpret_cast<char*>(buffer.data()), size);

			return buffer;
		}
	};
}
